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

    for (uint32_t i = 0; i < _ctx->net->_nplaces; ++i) {
        if (inQuery[i]) {
            vis_inc[i] = inQuery[i];
            vis_dec[i] = inQuery[i];
            queue.push_back(i);
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
        std::cout << *_ctx->net->placeNames()[i] << "#" << inQuery[i] << vis_inc[i] << vis_dec[i] << " ";
    }
    std::cout << ": " << ss.str() << "\n";

    _cache.insert(std::make_pair(query, visible));
    return _cache.at(query);
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
