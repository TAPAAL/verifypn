#include "PetriEngine/Extrapolator.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"

std::vector<std::vector<uint32_t>> PetriEngine::Extrapolator::findProducers(const PetriEngine::PetriNet *net) {
    // The PetriNet data structure does not allow us to find producers efficiently.
    // We (re)construct that information here since we will need it a lot for extrapolation.

    std::vector<std::vector<uint32_t>> producers(net->_nplaces);

    for (uint32_t i = 0; i < net->_ntransitions; ++i) {
        uint32_t finv = net->_transitions[i].outputs;
        uint32_t linv = net->_transitions[i+1].inputs;

        for ( ; finv < linv; ++finv) {
            const Invariant& inv = net->_invariants[finv];
            producers[inv.place].push_back(i);
        }
    }

    return producers;
}

std::vector<uint32_t> PetriEngine::Extrapolator::findUpperBounds(const PetriEngine::PetriNet *net) {
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

void PetriEngine::SimpleExtrapolator::init(const PetriEngine::PetriNet *net, const Condition *query) {
    _initialized = true;
    _net = net;
    _upperBounds = findUpperBounds(net);
    _producers = findProducers(net);
    _cache.clear();
}

void PetriEngine::SimpleExtrapolator::extrapolate(Marking *marking, Condition *query) {
    assert(_initialized);
    const auto visible = findVisiblePlaces(query);

    for (uint32_t i = 0; i < _net->_nplaces; ++i) {
        if (!visible[i]) {
            // Extrapolating below the upper bound may introduce behaviour
            uint32_t cur = marking->marking()[i];
            uint32_t ex = std::min(cur, _upperBounds[i]);
            _tokensExtrapolated += cur - ex;
            marking->marking()[i] = ex;
        }
    }
}

const std::vector<bool>& PetriEngine::SimpleExtrapolator::findVisiblePlaces(Condition *query) {
    auto it = _cache.find(query);
    if (it != _cache.end()) {
        return it->second;
    }

    PetriEngine::PQL::PlaceUseVisitor puv(_net->numberOfPlaces());
    PetriEngine::PQL::Visitor::visit(&puv, query);
    auto& inQuery = puv.in_use();

    std::vector<bool> vis_inc(_net->_nplaces); // Places where token increment is visible to query
    std::vector<bool> vis_dec(_net->_nplaces); // Places where token decrement is visible to query
    std::vector<uint32_t> queue;

    for (uint32_t i = 0; i < _net->_nplaces; ++i) {
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
            uint32_t t = _net->_placeToPtrs[p];
            uint32_t lastt = _net->_placeToPtrs[p + 1];

            for (; t < lastt; ++t) {
                // Put preset of postset in vis_inc,
                // and inhibiting preset of postset in vis_dec
                uint32_t finv = _net->_transitions[t].inputs;
                uint32_t linv = _net->_transitions[t].outputs;
                for ( ; finv < linv; ++finv) {
                    const Invariant& inv = _net->_invariants[finv];
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
            for (uint32_t t = 0; t < _producers[p].size(); ++t) {
                const TransPtr &ptr = _net->_transitions[_producers[p][t]];
                uint32_t finv = ptr.inputs;
                uint32_t linv = ptr.outputs;
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
    for (uint32_t i = 0; i < _net->_nplaces; ++i) {
        visible[i] = vis_inc[i] || vis_dec[i];
    }

    std::stringstream ss;
    query->toString(ss);
    std::cout << "Visible places : ";
    for (uint32_t i = 0; i < _net->_nplaces; ++i) {
        std::cout << *_net->placeNames()[i] << "#" << inQuery[i] << vis_inc[i] << vis_dec[i] << " ";
    }
    std::cout << ": " << ss.str() << "\n";

    _cache.insert(std::make_pair(query, visible));
    return _cache.at(query);
}

void PetriEngine::SmartExtrapolator::init(const PetriEngine::PetriNet *net, const PetriEngine::Condition *query) {
    // TODO
}

void PetriEngine::SmartExtrapolator::extrapolate(Marking *marking, Condition *query) {
    // TODO
}
