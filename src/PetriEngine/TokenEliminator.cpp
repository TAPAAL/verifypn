#include <queue>
#include "PetriEngine/TokenEliminator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
namespace PetriEngine {
    /***
     * The PlaceReachabilityDirectionVisitor analyzes which places are part of a a reachability query
     * and whether increases or decreases will bring us closer to satisfying the query.
     * The result is a vector of bytes where the IN_Q_DEC bit and/or the IN_Q_INC indicates if
     * and what direction is relevant.
     */
    class PlaceReachabilityDirectionVisitor : public PQL::Visitor {
    public:
        explicit PlaceReachabilityDirectionVisitor(size_t n_places) : _in_use(n_places) {}

        uint32_t operator[](size_t id) const {
            return _in_use[id];
        }

        [[nodiscard]] const std::vector<uint8_t>& get_result() const {
            return _in_use;
        }

    protected:
        void _accept(const PQL::NotCondition* element) override {
            Visitor::visit(this, (*element)[0]);
        }
        void _accept(const PQL::AndCondition* element) override {
            for (auto& e : *element) Visitor::visit(this, e);
        }
        void _accept(const PQL::OrCondition* element) override {
            for (auto& e : *element) Visitor::visit(this, e);
        }
        void _accept(const PQL::LessThanCondition* element) override {
            direction = IN_Q_DEC;
            Visitor::visit(this, (*element)[0]);
            direction = IN_Q_INC;
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::LessThanOrEqualCondition* element) override {
            direction = IN_Q_DEC;
            Visitor::visit(this, (*element)[0]);
            direction = IN_Q_INC;
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::EqualCondition* element) override {
            throw std::runtime_error("EqualCondition should not exist in compiled reachability expression");
            direction = IN_Q_INC | IN_Q_DEC;
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::NotEqualCondition* element) override {
            throw std::runtime_error("NotEqualCondition should not exist in compiled reachability expression");
            direction = IN_Q_INC | IN_Q_DEC;
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::UnfoldedIdentifierExpr* element) override {
            _in_use[element->offset()] |= direction;
        }
        void _accept(const PQL::PlusExpr* element) override {
            // TODO: Test this
            for(auto& p : element->places()) _in_use[p.first] |= direction;
        }
        void _accept(const PQL::MultiplyExpr* element) override {
            // TODO: Test this. Especially negative values
            for(auto& p : element->places()) _in_use[p.first] |= direction;
        }
        void _accept(const PQL::MinusExpr* element) override {
            // TODO: Do we need to negate here?
            Visitor::visit(this, (*element)[0]);
        }
        void _accept(const PQL::SubtractExpr* element) override {
            // TODO: Do we need to negate here?
            for(auto& e : element->expressions()) Visitor::visit(this, e);
        }
        void _accept(const PQL::CompareConjunction* element) override {
            // Compiled fireability proposition
            if (element->isNegated() == negated) {
                for (auto& e : *element) {
                    if (e._lower != 0) _in_use[e._place] |= IN_Q_INC;
                    if (e._upper != std::numeric_limits<uint32_t>::max()) _in_use[e._place] |= IN_Q_DEC;
                }
            } else {
                for (auto& e : *element) {
                    if (e._lower != 0) _in_use[e._place] |= IN_Q_DEC;
                    if (e._upper != std::numeric_limits<uint32_t>::max()) _in_use[e._place] |= IN_Q_INC;
                }
            }
        }
        void _accept(const PQL::UnfoldedUpperBoundsCondition* element) override {
            for(auto& p : element->places())
                _in_use[p._place] |= IN_Q_INC;
        }

        void _accept(const PQL::EFCondition* el) override {
            Visitor::visit(this, (*el)[0]);
        }
        void _accept(const PQL::EGCondition* el) override {
            throw std::runtime_error("EGCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::AGCondition* el) override {
            throw std::runtime_error("AGCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::AFCondition* el) override {
            throw std::runtime_error("AFCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::EXCondition* el) override {
            throw std::runtime_error("EXCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::AXCondition* el) override {
            throw std::runtime_error("AXCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::EUCondition* el) override {
            throw std::runtime_error("EUCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::AUCondition* el) override {
            throw std::runtime_error("AUCondition should not exist in compiled reachability expression");
        }
        void _accept(const PQL::LiteralExpr* element) override {
            // no-op
        }
        void _accept(const PQL::DeadlockCondition* element) override {
            // no-op, disallowed due to loop sensitivity
        }

    private:
        std::vector<uint8_t> _in_use;
        bool negated = false;
        uint8_t direction = 0;
    };

    void TokenEliminator::init(const PetriNet *net) {
        _cache.clear();
        _net = net;
        setupProducersAndConsumers();
        setupExtBounds();
    }

    void TokenEliminator::setupProducersAndConsumers() {
        _inhibpost.resize(0);
        _inhibpost.resize(_net->_nplaces);
        std::vector<std::pair<std::vector<trans_t>, std::vector<trans_t>>> tmp_places(_net->_nplaces);

        for (uint32_t t = 0; t < _net->_ntransitions; t++) {
            const TransPtr &ptr = _net->_transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net->_invariants[finv].inhibitor) {
                    _inhibpost[_net->_invariants[finv].place].push_back(t);
                } else {
                    tmp_places[_net->_invariants[finv].place].second.emplace_back(t, _net->_invariants[finv].direction);
                }
            }

            finv = linv;
            linv = _net->_transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                if (_net->_invariants[finv].direction > 0)
                    tmp_places[_net->_invariants[finv].place].first.emplace_back(t, _net->_invariants[finv].direction);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (const auto& p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _parcs = std::make_unique<trans_t[]>(ntrans);

        _prodcons = std::make_unique<place_t[]>(_net->_nplaces + 1);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net->_nplaces; ++p) {
            auto &pre = tmp_places[p].first;
            auto &post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _prodcons[p].producers = offset;
            offset += pre.size();
            _prodcons[p].consumers = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _parcs[tn + _prodcons[p].producers] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _parcs[tn + _prodcons[p].consumers] = post[tn];
            }

        }
        assert(offset == ntrans);
        _prodcons[p].producers = offset;
        _prodcons[p].consumers = offset;
    }

    void TokenEliminator::setupExtBounds() {
        _extBounds.assign(_net->_nplaces, 0);
        for (uint32_t i = 0; i < _net->_ntransitions; ++i) {
            uint32_t finv = _net->_transitions[i].inputs;
            uint32_t linv = _net->_transitions[i].outputs;

            for ( ; finv < linv; ++finv) {
                const Invariant& inv = _net->_invariants[finv];
                if (inv.inhibitor) {
                    _extBounds[inv.place] = std::max(_extBounds[inv.place], inv.tokens);
                }
            }
        }
    }

    std::pair<const TokenEliminator::trans_t*, const TokenEliminator::trans_t*> TokenEliminator::producers(uint32_t p) const {
        const place_t pmeta = _prodcons[p];
        return std::make_pair(&_parcs[pmeta.producers], &_parcs[pmeta.consumers]);
    }

    std::pair<const TokenEliminator::trans_t*, const TokenEliminator::trans_t*> TokenEliminator::consumers(uint32_t p) const {
        return std::make_pair(&_parcs[_prodcons[p].consumers], &_parcs[_prodcons[p + 1].producers]);
    }

    void TokenEliminator::eliminate(Marking *marking, Condition *query) {
        if (!_enabled) return;
        if (_doDynamic) {
            eliminateDynamic(marking, query);
        } else {
            eliminateStatic(marking, query);
        }
    }

    void TokenEliminator::findDeadPlacesAndTransitions(const Marking *marking) {

        _pflags.resize(_net->_nplaces);
        std::fill(_pflags.begin(), _pflags.end(), 0);
        _fireable.resize(_net->_ntransitions);
        std::fill(_fireable.begin(), _fireable.end(), false);

        std::queue<uint32_t> queue;

        // Helper functions

        auto processIncPlace = [&](uint32_t p) {
            if ((_pflags[p] & CAN_INC) == 0) {
                _pflags[p] |= CAN_INC;
                for (auto [t, last] = consumers(p); t < last; ++t) {
                    if (!_fireable[t->index])
                        queue.push(t->index);
                }
            }
        };

        auto processDecPlace = [&](uint32_t p) {
            if ((_pflags[p] & CAN_DEC) == 0) {
                _pflags[p] |= CAN_DEC;
                for (uint32_t t : _inhibpost[p]) {
                    if (!_fireable[t])
                        queue.push(t);
                }
            }
        };

        auto processEnabled = [&](uint32_t t) {
            _fireable[t] = true;
            // Find and process negative pre-set and positive post-set
            for (auto [finv, linv] = _net->preset(t); finv < linv; ++finv) {
                if (finv->direction < 0)
                    processDecPlace(finv->place);
            }
            for (auto [finv, linv] = _net->postset(t); finv < linv; ++finv) {
                if (finv->direction > 0)
                    processIncPlace(finv->place);
            }
        };

        // Process initially enabled transitions
        for (uint32_t t = 0; t < _net->_ntransitions; ++t) {
            uint32_t i = _net->_transitions[t].inputs;
            uint32_t fout = _net->_transitions[t].outputs;
            bool enabled = true;
            for ( ; i < fout; i++) {
                const Invariant& preinv = _net->_invariants[i];
                if (preinv.inhibitor != (preinv.tokens > (*marking)[preinv.place])) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Compute fixed point of effectively dead places and transitions

        while (!queue.empty()) {
            uint32_t t = queue.front();
            queue.pop();
            if (_fireable[t]) continue;

            // Is t enabled?
            bool enabled = true;
            uint32_t finv = _net->_transitions[t].inputs;
            uint32_t linv = _net->_transitions[t].outputs;
            for (; finv < linv; ++finv) {
                const Invariant& arc = _net->_invariants[finv];
                bool notInhibited = !arc.inhibitor || arc.tokens > (*marking)[arc.place] || (_pflags[arc.place] & CAN_DEC) > 0;
                bool enoughTokens = arc.inhibitor || arc.tokens <= (*marking)[arc.place] || (_pflags[arc.place] & CAN_INC) > 0;
                if (!notInhibited || !enoughTokens) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }
    }

    void TokenEliminator::eliminateDynamic(Marking * marking, Condition * query) {

        if (PQL::isLoopSensitive(query->shared_from_this()) || !PQL::isReachability(query)) {
            return;
        }

        std::stringstream before;
        if (_env_TOKEN_ELIM_DEBUG) {
            for (uint32_t i = 0; i < _net->_nplaces; i++) {
                before << (*marking)[i];
            }
        }

        findDeadPlacesAndTransitions(marking);
        findDynamicVisiblePlaces(query);

        for (uint32_t i = 0; i < _net->_nplaces; ++i) {
            if ((_pflags[i] & (VIS_INC | VIS_DEC | MUST_KEEP | IN_Q_INC | IN_Q_DEC)) == 0) {
                // Going below the upper bound may introduce behaviour
                uint32_t cur = marking->marking()[i];
                uint32_t ex = std::min(cur, _extBounds[i]);
                _tokensEliminated += cur - ex;
                marking->marking()[i] = ex;
            }
        }

        if (_env_TOKEN_ELIM_DEBUG) {
            std::stringstream after;
            for (uint32_t i = 0; i < _net->_nplaces; i++)
            {
                after << (*marking)[i];
            }
            if (before.str() == after.str())
                return;

            PQL::PlaceUseVisitor puv(_net->numberOfPlaces());
            PQL::Visitor::visit(&puv, query);
            auto& inQuery = puv.in_use();

            std::cout << before.str() << "->" << after.str() << " | Visible: ";
            for (uint32_t i = 0; i < _net->_nplaces; ++i) {
                if (inQuery[i] || (_pflags[i] & (VIS_INC | VIS_DEC | MUST_KEEP)) > 0) {
                    std::cout << *_net->placeNames()[i] << "#" << inQuery[i] << ((_pflags[i] & VIS_INC) > 0)
                              << ((_pflags[i] & VIS_DEC) > 0) << ((_pflags[i] & MUST_KEEP) > 0) << " ";
                }
            }
            std::cout << "| Live: ";
            for (uint32_t i = 0; i < _net->_nplaces; ++i) {
                if ((_pflags[i] & (CAN_INC | CAN_DEC)) > 0) {
                    std::cout << *_net->placeNames()[i] << "#" << ((_pflags[i] & CAN_INC) > 0)
                              << ((_pflags[i] & CAN_DEC) > 0) << " ";
                }
            }
            std::stringstream ss;
            query->toString(ss);
            std::cout << "| " << ss.str() << "\n";
        }
    }

    void TokenEliminator::findDynamicVisiblePlaces(Condition *query) {

        PlaceReachabilityDirectionVisitor pv(_net->numberOfPlaces());
        PQL::Visitor::visit(&pv, query);
        auto& use = pv.get_result();

        std::queue<uint32_t> queue;

        // Places in query are visible
        for (uint32_t p = 0; p < _net->_nplaces; ++p) {
            if (use[p] > 0) {
                _pflags[p] |= use[p];
                if ((_pflags[p] & IN_Q_DEC) > 0 && (_pflags[p] & CAN_DEC) > 0) {
                    _pflags[p] |= VIS_DEC;
                }
                if ((_pflags[p] & IN_Q_INC) > 0 && (_pflags[p] & CAN_INC) > 0) {
                    _pflags[p] |= VIS_INC;
                }
                queue.push(p);
            }
        }

        // Propagate visibility
        while (!queue.empty()) {
            uint32_t p = queue.front();
            queue.pop();

            if ((_pflags[p] & VIS_DEC) > 0) {
                // Put pre-set of negative consumers in vis_inc,
                // and inhibiting pre-set of negative consumers in vis_dec
                for (auto [t, last] = consumers(p); t < last; ++t) {
                    if (!_fireable[t->index] || t->direction >= 0) continue;
                    for (auto [finv, linv] = _net->preset(t->index) ; finv < linv; ++finv) {
                        const Invariant& arc = *finv;
                        if (arc.inhibitor) {
                            if ((_pflags[arc.place] & VIS_DEC) == 0 && (_pflags[arc.place] & CAN_DEC) > 0) {
                                queue.push(arc.place);
                                _pflags[arc.place] |= VIS_DEC;
                            }
                        } else {
                            if (arc.place != p && (_pflags[arc.place] & VIS_INC) == 0 && (_pflags[arc.place] & CAN_INC) > 0) {
                                queue.push(arc.place);
                                _pflags[arc.place] |= VIS_INC;
                            }
                            if (arc.tokens > 1 && (_pflags[arc.place] & CAN_INC) > 0) {
                                // This consumer may need more tokens to fire, so increases are also visible
                                _pflags[arc.place] |= VIS_INC;
                            }
                        }
                    }
                }
            }

            // This case is checker last since the first case may set VIS_INC for p
            if ((_pflags[p] & VIS_INC) > 0) {
                // Put pre-set of positive producers in vis_inc,
                // and inhibiting pre-set positive producers in vis_dec
                for (auto [t, last] = producers(p); t < last; ++t) {
                    if (!_fireable[t->index] || t->direction <= 0) continue;
                    for (auto [finv, linv] = _net->preset(t->index) ; finv < linv; ++finv) {
                        const Invariant& arc = *finv;
                        if (arc.inhibitor) {
                            if ((_pflags[arc.place] & VIS_DEC) == 0 && (_pflags[arc.place] & CAN_DEC) > 0) {
                                queue.push(arc.place);
                                _pflags[arc.place] |= VIS_DEC;
                            }
                        } else {
                            if ((_pflags[arc.place] & VIS_INC) == 0 && (_pflags[arc.place] & CAN_INC) > 0) {
                                queue.push(arc.place);
                                _pflags[arc.place] |= VIS_INC;
                            }
                        }
                    }
                }
            }
        }

        // We cannot disable fireable transitions affecting visible places, so their pre-set must be preserved,
        // even if their pre-set is effectively dead.
        for (uint32_t t = 0; t < _net->_ntransitions; ++t) {
            if (!_fireable[t]) continue;
            uint32_t finv = _net->_transitions[t].inputs;
            uint32_t linv = _net->_transitions[t+1].inputs;
            bool affectsVisible = false;
            for ( ; finv < linv; ++finv) {
                const Invariant& arc = _net->_invariants[finv];
                if (arc.direction > 0 && (_pflags[arc.place] & (VIS_INC | IN_Q_INC)) > 0) {
                    affectsVisible = true;
                    break;
                }
                if (arc.direction < 0 && (_pflags[arc.place] & (VIS_DEC | IN_Q_DEC)) > 0) {
                    affectsVisible = true;
                    break;
                }
            }
            if (affectsVisible) {
                finv = _net->_transitions[t].inputs;
                linv = _net->_transitions[t].outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& arc = _net->_invariants[finv];
                    if (!arc.inhibitor) {
                        _pflags[arc.place] |= MUST_KEEP;
                    }
                }
            }
        }
    }

    void TokenEliminator::eliminateStatic(Marking * marking, Condition * query) {
        const std::vector<bool> *visible;
        auto it = _cache.find(query);
        if (it != _cache.end()) {
            visible = &it->second;
        } else {
            visible = &findStaticVisiblePlaces(query);
        }

        if (!visible->empty()) {
            for (uint32_t i = 0; i < _net->_nplaces; ++i) {
                if (!(*visible)[i]) {
                    // Going below the upper bound may introduce behaviour
                    uint32_t cur = marking->marking()[i];
                    uint32_t ex = std::min(cur, _extBounds[i]);
                    _tokensEliminated += cur - ex;
                    marking->marking()[i] = ex;
                }
            }
        }
    }

    const std::vector<bool> &TokenEliminator::findStaticVisiblePlaces(Condition *query) {

        if (PQL::isLoopSensitive(query->shared_from_this()) || !PQL::isReachability(query)) {
            _cache.insert(std::make_pair(query, std::vector<bool>()));
            return _cache.at(query);
        }

        PlaceReachabilityDirectionVisitor puv(_net->numberOfPlaces());
        PQL::Visitor::visit(&puv, query);
        auto& use = puv.get_result();

        std::vector<bool> vis_inc(_net->_nplaces); // Places where token increment is visible to query
        std::vector<bool> vis_dec(_net->_nplaces); // Places where token decrement is visible to query
        std::vector<uint32_t> queue;

        // Places in query are visible
        for (uint32_t p = 0; p < _net->_nplaces; ++p) {
            if (use[p] > 0) {
                vis_inc[p] = (use[p] & IN_Q_INC) > 0;
                vis_dec[p] = (use[p] & IN_Q_DEC) > 0;
                queue.push_back(p);
            }
        }

        // Propagate visibility
        while (!queue.empty()) {
            uint32_t p = queue.back();
            queue.pop_back();

            if (vis_dec[p]) {
                // Put pre-set of negative consumers in vis_inc,
                // and inhibiting pre-set of negative consumers in vis_dec
                for (auto [t, last] = consumers(p); t < last; ++t) {
                    if (t->direction >= 0) continue;
                    for (auto [finv, fout] = _net->preset(t->index); finv < fout; ++finv) {
                        const Invariant& arc = *finv;
                        if (arc.inhibitor) {
                            if (!vis_dec[arc.place]) {
                                queue.push_back(arc.place);
                                vis_dec[arc.place] = true;
                            }
                        } else {
                            if (!vis_inc[arc.place] && arc.place != p) {
                                queue.push_back(arc.place);
                                vis_inc[arc.place] = true;
                            }
                            if (arc.tokens > 1) {
                                // This consumer may need more tokens to fire, so increases are also visible
                                vis_inc[p] = true;
                            }
                        }
                    }
                }
            }

            // This case is checker last since the first case may set VIS_INC for p
            if (vis_inc[p]) {
                // Put pre-set of positive producers in vis_inc,
                // and inhibiting pre-set of positive producers in vis_dec
                for (auto [t, last] = producers(p); t < last; ++t) {
                    if (t->direction <= 0) continue;
                    for (auto [finv, linv] = _net->preset(t->index) ; finv < linv; ++finv) {
                        const Invariant& arc = *finv;
                        if (arc.inhibitor) {
                            if (!vis_dec[arc.place]) {
                                queue.push_back(arc.place);
                                vis_dec[arc.place] = true;
                            }
                        } else {
                            if (!vis_inc[arc.place]) {
                                queue.push_back(arc.place);
                                vis_inc[arc.place] = true;
                            }
                        }
                    }
                }
            }
        }

        std::vector<bool> visible(_net->_nplaces);
        for (uint32_t i = 0; i < _net->_nplaces; ++i) {
            visible[i] = vis_inc[i] || vis_dec[i];
        }

        if (_env_TOKEN_ELIM_DEBUG) {
            std::stringstream ss;
            query->toString(ss);
            std::cout << "Visible places : ";
            for (uint32_t i = 0; i < _net->_nplaces; ++i) {
                if (use[i] > 0 || vis_inc[i] || vis_dec[i]) {
                    std::cout << *_net->placeNames()[i] << "#" << ((use[i] & (IN_Q_INC | IN_Q_DEC)) > 0) << vis_inc[i]
                              << vis_dec[i] << " ";
                }
            }
            std::cout << ": " << ss.str() << "\n";
        }

        _cache.insert(std::make_pair(query, visible));
        return _cache.at(query);
    }
}