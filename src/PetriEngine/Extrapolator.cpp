#include "PetriEngine/Extrapolator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"

std::vector<uint32_t> PetriEngine::Extrapolator::find_upper_bounds(const PetriEngine::PetriNet *net) {
    std::vector<uint32_t> bounds(net->_nplaces);
    for (int i; i < net->_ntransitions; ++i) {
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

void PetriEngine::SimpleExtrapolator::init(const PetriEngine::PetriNet *net, const Condition *query) {
    _initialized = true;
    _net = net;
    PetriEngine::PQL::PlaceUseVisitor puv(net->numberOfPlaces());
    PetriEngine::PQL::Visitor::visit(&puv, query);
    _inQuery = puv.in_use();
    _upperBounds = find_upper_bounds(net);
    _cache.clear();
}

void PetriEngine::SimpleExtrapolator::extrapolate(Marking *marking, Condition *query) {
    assert(_initialized);
    const auto visible = find_visible_places(query);

    for (int i = 0; i < _net->_nplaces; ++i) {
        if (!visible[i]) {
            // Extrapolating below the upper bound may introduce behaviour
            uint32_t cur = marking->marking()[i];
            uint32_t ex = std::min(cur, _upperBounds[i]);
            _tokensExtrapolated += cur - ex;
            marking->marking()[i] = ex;
        }
    }
}

const std::vector<bool>& PetriEngine::SimpleExtrapolator::find_visible_places(const Condition *query) {
    auto it = _cache.find(query);
    if (it != _cache.end()) {
        return it->second;
    }

    std::vector<bool> vis_inc(_net->_nplaces); // Places where token increment is visible to query
    std::vector<bool> vis_dec(_net->_nplaces); // Places where token decrement is visible to query
    std::vector<uint32_t> queue;

    for (int i = 0; i < _net->_nplaces; ++i) {
        if (_inQuery[i]) {
            vis_inc[i] = _inQuery[i];
            vis_dec[i] = _inQuery[i];
            queue.push_back(i);
        }
    }

    while (!queue.empty()) {
        uint32_t p = queue.back();
        queue.pop_back();

        uint32_t t = _net->_placeToPtrs[p];
        uint32_t tLast = _net->_placeToPtrs[p + 1];

        for (; t < tLast; ++t) {

            // Is transition consumer and/or producer?
            bool isConsumer = false;
            uint32_t finv = _net->_transitions[t].inputs;
            uint32_t linv = _net->_transitions[t].outputs;
            for ( ; finv < linv; ++finv) {
                if (_net->_invariants[finv].place == p && !_net->_invariants[finv].inhibitor) {
                    isConsumer = true;
                    break;
                }
            }
            bool isProducer = false;
            finv = _net->_transitions[t].outputs;
            linv = _net->_transitions[t + 1].inputs;
            for ( ; finv < linv; ++finv) {
                if (_net->_invariants[finv].place == p && !_net->_invariants[finv].inhibitor) {
                    isProducer = true;
                    break;
                }
            }

            if (vis_inc[p] && isProducer) {
                // Put preset of preset in vis_inc,
                // and inhibiting preset of preset in vis_dec
                const TransPtr &ptr = _net->_transitions[t];
                finv = ptr.inputs;
                linv = ptr.outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& inv = _net->_invariants[finv];
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

            if (vis_dec[p] && isConsumer) {
                // Put preset of postset in vis_inc,
                // and inhibiting preset of postset in vis_dec
                finv = _net->_transitions[t].outputs;
                linv = _net->_transitions[t + 1].inputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& inv = _net->_invariants[finv];
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

    std::vector<bool> visible(_net->_nplaces);
    for (int i = 0; i < _net->_nplaces; ++i) {
        visible[i] = vis_inc[i] || vis_dec[i];
    }
    _cache.insert(std::make_pair(query, visible));
    return _cache.at(query);
}

void PetriEngine::SmartExtrapolator::init(const PetriEngine::PetriNet *net, const PetriEngine::Condition *query) {
    // TODO
}

void PetriEngine::SmartExtrapolator::extrapolate(Marking *marking, Condition *query) {
    // TODO
}
