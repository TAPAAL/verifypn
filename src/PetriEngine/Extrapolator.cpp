#include <queue>
#include "PetriEngine/Extrapolator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
namespace PetriEngine {
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
            direction = VIS_DEC;
            Visitor::visit(this, (*element)[0]);
            direction = VIS_INC;
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::LessThanOrEqualCondition* element) override {
            direction = VIS_DEC;
            Visitor::visit(this, (*element)[0]);
            direction = VIS_INC;
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::EqualCondition* element) override {
            direction = VIS_INC | VIS_DEC;
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::NotEqualCondition* element) override {
            direction = VIS_INC | VIS_DEC;
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }
        void _accept(const PQL::UnfoldedIdentifierExpr* element) override {
            _in_use[element->offset()] |= direction | IN_Q;
        }
        void _accept(const PQL::PlusExpr* element) override {
            // TODO: Test this
            for(auto& p : element->places()) _in_use[p.first] |= direction | IN_Q;
        }
        void _accept(const PQL::MultiplyExpr* element) override {
            // TODO: Test this. Especially negative values
            for(auto& p : element->places()) _in_use[p.first] |= direction | IN_Q;
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
            // TODO: What even is this?
            for(auto& e : *element) _in_use[e._place] |= direction | IN_Q;
        }
        void _accept(const PQL::UnfoldedUpperBoundsCondition* element) override {
            for(auto& p : element->places())
                _in_use[p._place] |= VIS_INC | IN_Q;
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

        // Shallow elements, neither of these should exist in a compiled expression
        void _accept(const PQL::LiteralExpr* element) override {}
        void _accept(const PQL::DeadlockCondition* element) override {}

    private:
        std::vector<uint8_t> _in_use;
        bool negated = false;
        uint8_t direction = 0;
    };
}

PetriEngine::ExtrapolationContext::ExtrapolationContext(const PetriEngine::PetriNet *net) : net(net) {
    setupProducersAndConsumers();
    setupUpperBounds();
}

void PetriEngine::ExtrapolationContext::setupProducersAndConsumers() {
    // The PetriNet data structure does not allow us to go from a place to its producers and consumers.
    // We (re)construct that information here since we will need it a lot for extrapolation.

    producers.resize(net->_nplaces);
    consumers.resize(net->_nplaces);

    for (uint32_t i = 0; i < net->_ntransitions; ++i) {
        uint32_t a = net->_transitions[i].inputs;
        uint32_t outs = net->_transitions[i].outputs;
        uint32_t last = net->_transitions[i + 1].inputs;

        for ( ; a < outs; ++a) {
            const Invariant& inv = net->_invariants[a];
            consumers[inv.place].push_back(i);
        }

        for ( ; a < last; ++a) {
            const Invariant& inv = net->_invariants[a];
            producers[inv.place].push_back(i);
        }
    }
}

void PetriEngine::ExtrapolationContext::setupUpperBounds() {
    upperBounds.resize(net->_nplaces);
    for (uint32_t i = 0; i < net->_ntransitions; ++i) {
        uint32_t finv = net->_transitions[i].inputs;
        uint32_t linv = net->_transitions[i].outputs;

        for ( ; finv < linv; ++finv) {
            const Invariant& inv = net->_invariants[finv];
            if (inv.inhibitor) {
                upperBounds[inv.place] = std::max(upperBounds[inv.place], inv.tokens);
            }
        }
    }
}

int PetriEngine::ExtrapolationContext::effect(uint32_t t, uint32_t p) const {
    uint32_t i = net->_transitions[t].inputs;
    uint32_t fout = net->_transitions[t].outputs;
    int64_t w_rem = 0;
    for ( ; i < fout; ++i) {
        if (net->_invariants[i].place == p) {
            w_rem = net->_invariants[i].tokens;
            break;
        }
    }
    uint32_t j = fout;
    uint32_t end = net->_transitions[t+1].inputs;
    for ( ; j < end; ++j) {
        if (net->_invariants[j].place == p) {
            return net->_invariants[j].tokens - w_rem;
        }
    }
    return -w_rem;
}

void PetriEngine::SimpleReachExtrapolator::extrapolate(PetriEngine::Marking *marking, PetriEngine::Condition *query) {
    const std::vector<bool> *visible;
    auto it = _cache.find(query);
    if (it != _cache.end()) {
        visible = &it->second;
    } else {
        visible = &findVisiblePlaces(query);
    }

    if (!visible->empty()) {
        for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
            if (!(*visible)[i]) {
                // Extrapolating below the upper bound may introduce behaviour
                uint32_t cur = marking->marking()[i];
                uint32_t ex = std::min(cur, _ctx->upperBounds[i]);
                _tokensExtrapolated += cur - ex;
                marking->marking()[i] = ex;
            }
        }
    }
}

const std::vector<bool> &PetriEngine::SimpleReachExtrapolator::findVisiblePlaces(PetriEngine::Condition *query) {

    if (PQL::isLoopSensitive(query->shared_from_this()) || !PQL::isReachability(query)) {
        _cache.insert(std::make_pair(query, std::vector<bool>()));
        return _cache.at(query);
    }

    PlaceReachabilityDirectionVisitor puv(_ctx->net->numberOfPlaces());
    PQL::Visitor::visit(&puv, query);
    auto& use = puv.get_result();

    std::vector<bool> vis_inc(_ctx->net->_nplaces); // Places where token increment is visible to query
    std::vector<bool> vis_dec(_ctx->net->_nplaces); // Places where token decrement is visible to query
    std::vector<uint32_t> queue;

    for (uint32_t p = 0; p < _ctx->net->_nplaces; ++p) {
        if (use[p] > 0) {
            vis_inc[p] = (use[p] & VIS_INC) > 0;
            vis_dec[p] = (use[p] & VIS_DEC) > 0;
            queue.push_back(p);
        }
    }

    while (!queue.empty()) {
        uint32_t p = queue.back();
        queue.pop_back();

        if (vis_dec[p]) {
            // Put pre-set of negative post-set in vis_inc,
            // and inhibiting pre-set of post-set in vis_dec
            for (auto t : _ctx->consumers[p]) {
                if (_ctx->effect(t, p) >= 0) continue;
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t i = ptr.inputs;
                uint32_t fout = ptr.outputs;
                for ( ; i < fout; ++i) {
                    const Invariant& arc = _ctx->net->_invariants[i];
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

        if (vis_inc[p]) {
            // Put pre-set of positive pre-set in vis_inc,
            // and inhibiting pre-set of pre-set in vis_dec
            for (auto t : _ctx->producers[p]) {
                if (_ctx->effect(t, p) <= 0) continue;
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& inv = _ctx->net->_invariants[finv];
                    if (inv.inhibitor) {
                        if (!vis_dec[inv.place]) {
                            queue.push_back(inv.place);
                            vis_dec[inv.place] = true;
                        }
                    } else {
                        if (!vis_inc[inv.place]) {
                            queue.push_back(inv.place);
                            vis_inc[inv.place] = true;
                        }
                    }
                }
            }
        }
    }

    std::vector<bool> visible(_ctx->net->_nplaces);
    for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
        visible[i] = vis_inc[i] || vis_dec[i];
    }

    std::stringstream ss;
    query->toString(ss);
    std::cout << "Visible places : ";
    for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
        if (use[i] > 0 || vis_inc[i] || vis_dec[i]) {
            std::cout << *_ctx->net->placeNames()[i] << "#" << ((use[i] & IN_Q) > 0) << vis_inc[i] << vis_dec[i] << " ";
        }
    }
    std::cout << ": " << ss.str() << "\n";

    _cache.insert(std::make_pair(query, visible));
    return _cache.at(query);
}

void PetriEngine::DynamicReachExtrapolator::extrapolate(PetriEngine::Marking *marking, PetriEngine::Condition *query) {

    if (PQL::isLoopSensitive(query->shared_from_this()) || !PQL::isReachability(query)) {
        return;
    }

    std::stringstream before;
    for (uint32_t i = 0; i < _ctx->net->_nplaces; i++)
    {
        before << (*marking)[i];
    }

    findDeadPlacesAndTransitions(marking);
    findVisiblePlaces(query);

    for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
        if ((_pflags[i] & (VIS_INC | VIS_DEC | MUST_KEEP)) == 0) {
            // Extrapolating below the upper bound may introduce behaviour
            uint32_t cur = marking->marking()[i];
            uint32_t ex = std::min(cur, _ctx->upperBounds[i]);
            _tokensExtrapolated += cur - ex;
            marking->marking()[i] = ex;
        }
    }

    if (std::getenv("DYN_EXTRAP_DEBUG") != nullptr) {
        std::stringstream after;
        for (uint32_t i = 0; i < _ctx->net->_nplaces; i++)
        {
            after << (*marking)[i];
        }
        if (before.str() == after.str())
            return;

        PetriEngine::PQL::PlaceUseVisitor puv(_ctx->net->numberOfPlaces());
        PetriEngine::PQL::Visitor::visit(&puv, query);
        auto& inQuery = puv.in_use();

        std::cout << before.str() << "->" << after.str() << " | Visible: ";
        for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
            if (inQuery[i] || (_pflags[i] & (VIS_INC | VIS_DEC | MUST_KEEP)) > 0) {
                std::cout << *_ctx->net->placeNames()[i] << "#" << inQuery[i] << ((_pflags[i] & VIS_INC) > 0)
                          << ((_pflags[i] & VIS_DEC) > 0) << ((_pflags[i] & MUST_KEEP) > 0) << " ";
            }
        }
        std::cout << "| Live: ";
        for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
            if ((_pflags[i] & (CAN_INC | CAN_DEC)) > 0) {
                std::cout << *_ctx->net->placeNames()[i] << "#" << ((_pflags[i] & CAN_INC) > 0)
                          << ((_pflags[i] & CAN_DEC) > 0) << " ";
            }
        }
        std::stringstream ss;
        query->toString(ss);
        std::cout << "| " << ss.str() << "\n";
    }
}

void PetriEngine::DynamicReachExtrapolator::findDeadPlacesAndTransitions(const PetriEngine::Marking *marking) {

    _pflags.resize(_ctx->net->_nplaces);
    std::fill(_pflags.begin(), _pflags.end(), 0);
    _fireable.resize(_ctx->net->_ntransitions);
    std::fill(_fireable.begin(), _fireable.end(), false);

    std::queue<uint32_t> queue;

    // Helper functions

    auto processIncPlace = [&](uint32_t p) {
        if ((_pflags[p] & CAN_INC) == 0) {
            _pflags[p] |= CAN_INC;
            for (uint32_t t : _ctx->consumers[p]) {
                if (!_fireable[t])
                    queue.push(t);
            }
        }
    };

    auto processDecPlace = [&](uint32_t p) {
        if ((_pflags[p] & CAN_DEC) == 0) {
            _pflags[p] |= CAN_DEC;
            for (uint32_t t : _ctx->consumers[p]) {
                if (!_fireable[t])
                    queue.push(t);
            }
        }
    };

    auto processEnabled = [&](uint32_t t) {
        _fireable[t] = true;
        // Find and process negative pre-set and positive post-set
        uint32_t i = _ctx->net->_transitions[t].inputs;
        uint32_t fout = _ctx->net->_transitions[t].outputs;
        uint32_t j = fout;
        uint32_t end = _ctx->net->_transitions[t+1].inputs;
        while (i < fout && j < end)
        {
            const Invariant& preinv = _ctx->net->_invariants[i];
            const Invariant& postinv = _ctx->net->_invariants[j];

            if (preinv.place < postinv.place) {
                if (!preinv.inhibitor)
                    processDecPlace(preinv.place);
                i++;
            } else if (preinv.place > postinv.place) {
                processIncPlace(postinv.place);
                j++;
            } else {
                if (preinv.inhibitor) {
                    processIncPlace(postinv.place);
                } else {
                    // There are both an in and an out arc to this place. Is the effect non-zero?
                    if (preinv.tokens > postinv.tokens) {
                        processDecPlace(preinv.place);
                    } else if (preinv.tokens < postinv.tokens) {
                        processIncPlace(postinv.place);
                    }
                }

                i++; j++;
            }
        }
        for ( ; i < fout; i++) {
            const Invariant& preinv = _ctx->net->_invariants[i];
            if (!preinv.inhibitor)
                processDecPlace(preinv.place);
        }
        for ( ; j < end; j++) {
            processIncPlace(_ctx->net->_invariants[j].place);
        }
    };

    // Process initially enabled transitions
    for (uint32_t t = 0; t < _ctx->net->_ntransitions; ++t) {
        uint32_t i = _ctx->net->_transitions[t].inputs;
        uint32_t fout = _ctx->net->_transitions[t].outputs;
        bool enabled = true;
        for ( ; i < fout; i++) {
            const Invariant& preinv = _ctx->net->_invariants[i];
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
        uint32_t finv = _ctx->net->_transitions[t].inputs;
        uint32_t linv = _ctx->net->_transitions[t].outputs;
        for (; finv < linv; ++finv) {
            const Invariant& arc = _ctx->net->_invariants[finv];
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

void PetriEngine::DynamicReachExtrapolator::findVisiblePlaces(PetriEngine::Condition *query) {

    PlaceReachabilityDirectionVisitor puv(_ctx->net->numberOfPlaces());
    PQL::Visitor::visit(&puv, query);
    auto& use = puv.get_result();

    std::queue<uint32_t> queue;

    for (uint32_t p = 0; p < _ctx->net->_nplaces; ++p) {
        if (use[p] > 0) {
            _pflags[p] |= use[p];
            queue.push(p);
        }
    }

    while (!queue.empty()) {
        uint32_t p = queue.front();
        queue.pop();

        if ((_pflags[p] & VIS_DEC) > 0) {
            // Put pre-set of negative post-set in vis_inc,
            // and inhibiting pre-set of post-set in vis_dec
            for (auto t : _ctx->consumers[p]) {
                if (!_fireable[t] || _ctx->effect(t, p) >= 0) continue;
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& arc = _ctx->net->_invariants[finv];
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

        if ((_pflags[p] & VIS_INC) > 0) {
            // Put pre-set of positive pre-set in vis_inc,
            // and inhibiting pre-set of pre-set in vis_dec
            for (auto t : _ctx->producers[p]) {
                if (!_fireable[t] || _ctx->effect(t, p) <= 0) continue;
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& arc = _ctx->net->_invariants[finv];
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
    for (uint32_t t = 0; t < _ctx->net->_ntransitions; ++t) {
        if (!_fireable[t]) continue;
        uint32_t finv = _ctx->net->_transitions[t].inputs;
        uint32_t linv = _ctx->net->_transitions[t+1].inputs;
        bool affectsVisible = false;
        for ( ; finv < linv; ++finv) {
            const Invariant& arc = _ctx->net->_invariants[finv];
            if ((_pflags[arc.place] & (VIS_INC | VIS_DEC)) > 0) {
                affectsVisible = true;
                break;
            }
        }
        if (affectsVisible) {
            finv = _ctx->net->_transitions[t].inputs;
            linv = _ctx->net->_transitions[t].outputs;
            for ( ; finv < linv; ++finv) {
                const Invariant& arc = _ctx->net->_invariants[finv];
                if (!arc.inhibitor) {
                    _pflags[arc.place] |= MUST_KEEP;
                }
            }
        }
    }
}

void PetriEngine::AdaptiveExtrapolator::init(const PetriEngine::PetriNet *net, const PetriEngine::Condition *query) {
    if (!_ctx) {
        _ctx = std::make_shared<ExtrapolationContext>(net);
    }
    _simple.initWithCtx(_ctx, query);
    //_ctl.initWithCtx(_ctx, query);
}

void PetriEngine::AdaptiveExtrapolator::extrapolate(PetriEngine::Marking *marking, PetriEngine::Condition *query) {
    _simple.extrapolate(marking, query);
}

size_t PetriEngine::AdaptiveExtrapolator::tokensExtrapolated() const {
    return _simple.tokensExtrapolated(); //+ _ctl.tokensExtrapolated(); TODO
}
