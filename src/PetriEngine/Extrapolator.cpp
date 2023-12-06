#include <queue>
#include "PetriEngine/Extrapolator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

PetriEngine::ExtrapolationContext::ExtrapolationContext(const PetriEngine::PetriNet *net) : net(net) {
    std::tie(producers, consumers) = findProducersAndConsumers(net);
    upperBounds = findUpperBounds(net);
}

std::pair<std::vector<std::vector<uint32_t>>, std::vector<std::vector<uint32_t>>>
PetriEngine::ExtrapolationContext::findProducersAndConsumers(const PetriEngine::PetriNet *net) {
    // The PetriNet data structure does not allow us to go from a place to its producers and consumers.
    // We (re)construct that information here since we will need it a lot for extrapolation.

    std::vector<std::vector<uint32_t>> producers(net->_nplaces);
    std::vector<std::vector<uint32_t>> consumers(net->_nplaces);

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

    return { producers, consumers };
}

std::vector<uint32_t> PetriEngine::ExtrapolationContext::findUpperBounds(const PetriEngine::PetriNet *net) {
    std::vector<uint32_t> bounds(net->_nplaces);
    for (uint32_t i = 0; i < net->_ntransitions; ++i) {
        uint32_t finv = net->_transitions[i].inputs;
        uint32_t linv = net->_transitions[i].outputs;

        for ( ; finv < linv; ++finv) {
            const Invariant& inv = net->_invariants[finv];
            if (inv.inhibitor) {
                bounds[inv.place] = std::max(bounds[inv.place], inv.tokens);
            }
        }
    }

    return bounds;
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

    PetriEngine::PQL::PlaceUseVisitor puv(_ctx->net->numberOfPlaces());
    PetriEngine::PQL::Visitor::visit(&puv, query);
    auto& inQuery = puv.in_use();

    std::vector<bool> vis_inc(_ctx->net->_nplaces); // Places where token increment is visible to query
    std::vector<bool> vis_dec(_ctx->net->_nplaces); // Places where token decrement is visible to query
    std::vector<uint32_t> queue;

    for (uint32_t p = 0; p < _ctx->net->_nplaces; ++p) {
        if (inQuery[p]) {
            vis_inc[p] = inQuery[p];
            vis_dec[p] = inQuery[p];
            queue.push_back(p);
        }
    }

    while (!queue.empty()) {
        uint32_t p = queue.back();
        queue.pop_back();

        if (vis_dec[p]) {
            // Put preset of postset in vis_inc,
            // and inhibiting preset of postset in vis_dec
            for (auto t : _ctx->consumers[p]) {
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& inv = _ctx->net->_invariants[finv];
                    if (inv.place == p) {
                        continue;
                    }
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
                        if (inv.tokens > 1) {
                            // This consumer may need more tokens to fire, so increases are also visible
                            vis_inc[p] = true;
                        }
                    }
                }
            }
        }

        if (vis_inc[p]) {
            // Put preset of preset in vis_inc,
            // and inhibiting preset of preset in vis_dec
            for (auto t : _ctx->producers[p]) {
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
        if (inQuery[i] || vis_inc[i] || vis_dec[i]) {
            std::cout << *_ctx->net->placeNames()[i] << "#" << inQuery[i] << vis_inc[i] << vis_dec[i] << " ";
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

    findDeadPlacesAndTransitions(marking);
    findVisiblePlaces(query);

    for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
        if ((_pflags[i] & (VIS_INC | VIS_DEC)) == 0) {
            // Extrapolating below the upper bound may introduce behaviour
            uint32_t cur = marking->marking()[i];
            uint32_t ex = std::min(cur, _ctx->upperBounds[i]);
            _tokensExtrapolated += cur - ex;
            marking->marking()[i] = ex;
        }
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
        // Find and process negative preset and positive postset
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

    PetriEngine::PQL::PlaceUseVisitor puv(_ctx->net->numberOfPlaces());
    PetriEngine::PQL::Visitor::visit(&puv, query);
    auto& inQuery = puv.in_use();

    std::queue<uint32_t> queue;

    for (uint32_t p = 0; p < _ctx->net->_nplaces; ++p) {
        if (inQuery[p]) {
            _pflags[p] |= VIS_INC;
            _pflags[p] |= VIS_DEC;
            queue.push(p);
        }
    }

    while (!queue.empty()) {
        uint32_t p = queue.front();
        queue.pop();

        if ((_pflags[p] & VIS_DEC) > 0) {
            // Put preset of postset in vis_inc,
            // and inhibiting preset of postset in vis_dec
            for (auto t : _ctx->consumers[p]) {
                if (!_fireable[t]) continue;
                const TransPtr &ptr = _ctx->net->_transitions[t];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& arc = _ctx->net->_invariants[finv];
                    if (arc.place == p) {
                        continue;
                    }
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
                        if (arc.tokens > 1 && (_pflags[arc.place] & CAN_INC) > 0) {
                            // This consumer may need more tokens to fire, so increases are also visible
                            _pflags[arc.place] |= VIS_INC;
                        }
                    }
                }
            }
        }

        if ((_pflags[p] & VIS_INC) > 0) {
            // Put preset of preset in vis_inc,
            // and inhibiting preset of preset in vis_dec
            for (auto t : _ctx->producers[p]) {
                if (!_fireable[t]) continue;
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

    if (std::getenv("DYN_EXTRAP_DEBUG") != nullptr) {
//        for (uint32_t i = 0; i < n_places; i++)
//        {
//            std::cout << marking[i];
//        }
        std::cout << " | Visible: ";
        for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
            if (inQuery[i] || (_pflags[i] & (VIS_INC | VIS_DEC)) > 0) {
                std::cout << *_ctx->net->placeNames()[i] << "#" << inQuery[i] << ((_pflags[i] & VIS_INC) > 0)
                          << ((_pflags[i] & VIS_DEC) > 0) << " ";
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
