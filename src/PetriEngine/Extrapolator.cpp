#include "PetriEngine/Extrapolator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"

void PetriEngine::SimpleExtrapolator::init(const PetriEngine::PetriNet *net, const Condition *query) {
    _initialized = true;
    _net = net;
    PetriEngine::PQL::PlaceUseVisitor puv(net->numberOfPlaces());
    PetriEngine::PQL::Visitor::visit(&puv, query);
    _inQuery = puv.in_use();
    _cache.clear();
}

void PetriEngine::SimpleExtrapolator::extrapolate(Marking *marking, Condition *query) {
    assert(_initialized);
    const auto live_places = find_visible_places(query);

    for (int i = 0; i < _net->_nplaces; ++i) {
        if (!live_places[i]) {
            marking->marking()[i] = 0; // TODO: Minimum of current and greatest inhibitor weight
        }
    }
}

std::vector<bool> PetriEngine::SimpleExtrapolator::find_visible_places(const Condition *query) {
    auto it = _cache.find(query);
    if (it != _cache.end()) {
        return it->second;
    }

    std::vector<bool> vis_inc(_net->_nplaces);
    std::vector<bool> vis_dec(_net->_nplaces);
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
    return visible;
}

void PetriEngine::SmartExtrapolator::init(const PetriEngine::PetriNet *net, const PetriEngine::Condition *query) {
    // TODO
}

void PetriEngine::SmartExtrapolator::extrapolate(Marking *marking, Condition *query) {
    // TODO
}
