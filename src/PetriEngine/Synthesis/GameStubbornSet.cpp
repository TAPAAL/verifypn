
#include "PetriEngine/Synthesis/GameStubbornSet.h"
#include "PetriEngine/Synthesis/IntervalVisitor.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "PetriEngine/PQL/Evaluation.h"

#include <stack>


namespace PetriEngine {
    namespace Synthesis {

        GameStubbornSet::GameStubbornSet(const PetriNet& net, PQL::Condition* predicate, bool is_safety)
        : StubbornSet(net, predicate), _is_safety(is_safety), _in_query(net.numberOfPlaces()) {
            for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
                if (_net.controllable(t))
                    _reach_actions.emplace_back(t);
                else
                    _avoid_actions.emplace_back(t);
            }
            if (_is_safety)
                std::swap(_reach_actions, _avoid_actions);
            computeSafe();
            _inhibiting_place.resize(_net.numberOfPlaces(), false);
            _future_enabled.resize(_net.numberOfTransitions(), false);
            for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
                auto [finv, linv] = _net.preset(t);
                for (; finv < linv; ++finv)
                    if (finv->inhibitor)
                        _inhibiting_place[finv->place] = true;
            }
            PQL::Visitor::visit(_in_query, predicate);
            _fireing_bounds = std::make_unique<uint32_t[]>(_net.numberOfTransitions());
            _place_bounds = std::make_unique<std::pair<uint32_t,uint32_t>[]>(_net.numberOfPlaces());
        }

        void GameStubbornSet::addToStub(uint32_t t) {
            if (!_stubborn[t]) {
                if (_enabled[t])
                    _added_enabled = true;
                if (_enabled[t] && !_safe_actions[t])
                    _added_unsafe = true;
                else {
                    if (_env_acts.empty() == _net.controllable(t) &&
                        !_future_enabled[t])
                        return;
                    StubbornSet::addToStub(t);
                }
            }
        }

        uint32_t GameStubbornSet::next_env() {
            while (!_env_acts.empty()) {
                auto r = _env_acts.front();
                _env_acts.pop_front();
                if (_stubborn[r]) return r;
            }
            return std::numeric_limits<uint32_t>::max();
        }

        uint32_t GameStubbornSet::next_ctrl() {
            while (!_ctrl_acts.empty()) {
                auto r = _ctrl_acts.front();
                _ctrl_acts.pop_front();
                if (_stubborn[r]) return r;
            }
            return std::numeric_limits<uint32_t>::max();
        }

        void GameStubbornSet::skip() {
            set_all_stubborn();
        }

        void GameStubbornSet::computeSafe() {
            _safe_actions.resize(_net.numberOfTransitions(), true);
            _safe_places.resize(_net.numberOfPlaces(), true);
            std::fill(_safe_actions.begin(), _safe_actions.end(), true);
            for (uint32_t t = 0; t < _net.numberOfTransitions(); ++t) {
                if (_net.controllable(t))
                    continue;
                // check if the (t+)* contains an unctrl or
                // (t-)* has unctrl with inhib
                auto pre = _net.preset(t);
                for (; pre.first != pre.second; ++pre.first) {
                    // check ways we can enable this transition by ctrl
                    if (pre.first->inhibitor) {
                        auto it = _places[pre.first->place].post;
                        auto end = _places[pre.first->place + 1].pre;
                        for (; it != end; ++it) {
                            if (_arcs[it].direction < 0) {
                                auto id = _arcs[it].index;
                                if (!_net.controllable(id))
                                    continue;
                                // has to be consuming
                                _safe_actions[id] = false;
                                break;
                            }
                        }
                    } else {
                        auto it = _places[pre.first->place].pre;
                        auto end = _places[pre.first->place].post;
                        for (; it != end; ++it) {
                            if (_arcs[it].direction > 0) {
                                auto id = _arcs[it].index;
                                if (!_net.controllable(id))
                                    continue;
                                _safe_actions[id] = false;
                                break;
                            }
                        }
                    }
                    if (!_safe_actions[t]) {
                        for (auto pp = _net.postset(t); pp.first != pp.second; ++pp.first) {
                            _safe_places[pp.first->place] = false;
                        }
                        break;
                    }
                }
            }
        }

        void GameStubbornSet::computeBounds() {
            std::vector<uint32_t> waiting;
            auto handle_transition = [this, &waiting](size_t t) {
                if (!_future_enabled[t])
                    return;
                auto mx = std::numeric_limits<uint32_t>::max();
                auto [finv, linv] = _net.preset(t);
                auto [fout, lout] = _net.postset(t);
                for (; finv < linv; ++finv) {
                    if (finv->direction < 0 && !finv->inhibitor) {
                        if (_place_bounds[finv->place].second == std::numeric_limits<uint32_t>::max())
                            continue;
                        while (fout < lout && fout->place < finv->place)
                            ++fout;
                        if (fout < lout && fout->place == finv->place) {
                            mx = std::min(_place_bounds[finv->place].second / (fout->tokens - finv->tokens), mx);
                        } else {
                            mx = std::min(_place_bounds[finv->place].second / finv->tokens, mx);
                        }
                    }
                }
                if (_fireing_bounds[t] != mx) {
                    assert(_fireing_bounds[t] >= mx);
                    _fireing_bounds[t] = mx;
                    auto [fout, lout] = _net.postset(t);
                    for (; fout < lout; ++fout) {
                        if (fout->direction > 0 && (_places_seen[fout->place] & WAITING) == 0) {
                            _places_seen[fout->place] |= WAITING;
                            waiting.push_back(fout->place);
                        }
                    }
                }
            };

            auto handle_place = [this, &handle_transition](size_t p) {
                if (_place_bounds[p].second == 0)
                    return;
                _places_seen[p] &= ~WAITING;

                // place loop
                uint64_t sum = 0;
                for (auto ti = _places[p].pre; ti != _places[p].post; ++ti) {
                    const auto& arc = _arcs[ti];
                    if (arc.direction <= 0 ||
                        _fireing_bounds[arc.index] == 0)
                        continue;
                    if (_fireing_bounds[arc.index] == std::numeric_limits<uint32_t>::max()) {
                        assert(_place_bounds[p].second == std::numeric_limits<uint32_t>::max());
                        return;
                    }
                    assert(_future_enabled[arc.index]);
                    auto [finv, linv] = _net.preset(arc.index);
                    auto [fout, lout] = _net.postset(arc.index);
                    for (; fout < lout; ++fout) {
                        if (fout->place != p)
                            continue;
                        while (finv < linv && finv->place < fout->place) ++finv;
                        auto take = 0;
                        if (finv < linv && finv->place == p && !finv->inhibitor) {
                            take = finv->tokens;
                        }
                        sum += (fout->tokens - take) * _fireing_bounds[arc.index];
                        break;
                    }
                }
                assert(sum <= _place_bounds[p].second);
                if (_place_bounds[p].second != sum) {
                    _place_bounds[p].second = sum;
                    for (auto ti = _places[p].post; ti != _places[p + 1].pre; ++ti) {
                        if (_arcs[ti].direction < 0)
                            handle_transition(_arcs[ti].index);
                    }
                }

            };

            // initialize places
            for (size_t p = 0; p < _net.numberOfPlaces(); ++p) {
                uint32_t ub = (*_parent)[p];
                for (auto ti = _places[p].pre; ti != _places[p].post; ++ti) {
                    const auto& arc = _arcs[ti];
                    if (arc.direction <= 0 || !_future_enabled[arc.index])
                        continue;
                    ub = std::numeric_limits<uint32_t>::max();
                    break;
                }
                _place_bounds[p] = std::make_pair((*_parent)[p], ub);
            }
            // initialize counters
            for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
                if (_future_enabled[t]) {
                    _fireing_bounds[t] = std::numeric_limits<uint32_t>::max();
                    handle_transition(t);
                } else
                    _fireing_bounds[t] = 0;
            }

            while (!waiting.empty()) {
                auto p = waiting.back();
                waiting.pop_back();
                handle_place(p);
            }
            for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
                auto [finv, linv] = _net.preset(t);
                auto [fout, lout] = _net.postset(t);
                for (; finv < linv; ++finv) {
                    if (finv->direction >= 0 || finv->inhibitor) continue;
                    uint64_t take = finv->tokens;
                    if (_fireing_bounds[t] == std::numeric_limits<uint32_t>::max()) {
                        _place_bounds[finv->place].first = 0;
                        continue;
                    }
                    while (fout < lout && fout->place < finv->place) ++fout;
                    if (fout < lout && fout->place == finv->place)
                        take -= fout->tokens;
                    assert(take > 0);
                    take *= _fireing_bounds[t];
                    if (take >= _place_bounds[finv->place].first)
                        _place_bounds[finv->place].first = 0;
                    else
                        _place_bounds[finv->place].first -= take;
                }
            }
        }

        bool GameStubbornSet::approximateFuture(const bool ctrl) {
            std::stack<uint32_t> waiting;
            std::fill(_future_enabled.begin(), _future_enabled.end(), false);
            auto color_transition = [this, &waiting](auto t) {
                _future_enabled[t] = true;
                if (_netContainsInhibitorArcs) {
                    // check for decrementors
                    auto [finv, linv] = _net.preset(t);

                    for (; finv < linv; ++finv) {
                        if (finv->direction < 0 && _inhibiting_place[finv->place]) {
                            if ((_places_seen[finv->place] & DECR) == 0)
                                waiting.push(finv->place);
                            _places_seen[finv->place] |= DECR;
                        }
                    }
                }
                {
                    // color incrementors
                    auto [finv, linv] = _net.postset(t);

                    for (; finv < linv; ++finv) {
                        if (finv->direction > 0) {
                            if ((_places_seen[finv->place] & INCR) == 0)
                                waiting.push(finv->place);
                            _places_seen[finv->place] |= INCR;
                        }
                    }
                }
            };

            // bootstrap
            for (auto t : (ctrl ? _ctrl_acts : _env_acts)) {
                assert(_enabled[t]);
                color_transition(t);
            }

            // saturate
            bool seen_from_query = false;
            while (!waiting.empty()) {
                auto p = waiting.top();
                waiting.pop();
                if (_in_query[p]) {
                    seen_from_query = true;
                }
                if ((_places_seen[p] & INCR) != 0 ||
                    (_inhibiting_place[p] && (_places_seen[p] & DECR) != 0)) {
                    auto pf = _places[p].post;
                    auto pl = _places[p + 1].pre;
                    for (; pf < pl; ++pf) {
                        if ((_future_enabled[_arcs[pf].index]) == 0 &&
                            _net.controllable(_arcs[pf].index) == ctrl) {
                            auto t = _arcs[pf].index;
                            auto [finv, linv] = _net.preset(t);
                            bool ok = true;
                            // check color of preset
                            for (; finv < linv; ++finv) {
                                if (!finv->inhibitor &&
                                    (_places_seen[finv->place] & INCR) == 0 &&
                                    (*_parent)[finv->place] < finv->tokens) {
                                    // no proof of enabelable yet
                                    ok = false;
                                    break;
                                }
                                if (finv->inhibitor &&
                                    (_places_seen[finv->place] & DECR) == 0 &&
                                    (*_parent)[finv->place] >= finv->tokens) {
                                    // no proof of uninhib yet
                                    ok = false;
                                    break;
                                }
                            }
                            if (ok) {
                                color_transition(t);
                            }
                        }
                    }
                }
            }
            return seen_from_query;
        }

        void GameStubbornSet::reset() {
            StubbornSet::reset();
            _ctrl_acts.clear();
            _env_acts.clear();
            _added_unsafe = false;
            _added_enabled = false;
        }

        bool GameStubbornSet::prepare(const Structures::State *marking) {
            reset();
            _parent = marking;
            StubbornSet::constructEnabled([this](uint32_t t) {
                if (_net.controllable(t))
                    _ctrl_acts.push_back(t);
                else
                    _env_acts.push_back(t);
                return true;
            });
            if (!_ctrl_acts.empty() &&
                !_env_acts.empty()) {
                skip();
                return true;
            }

            if (_nenabled <= 1) {
                if (_nenabled == 1)
                    _stubborn[_ordering.front()] = true;
                return true;
            }

            const auto touches_query = approximateFuture(!_ctrl_acts.empty());
            const bool reach_player = _is_safety ? _ctrl_acts.empty() : _env_acts.empty();
            if (!reach_player) {
                // TODO; approximate forward reach to avoid accidential
                // acceptance
                if (touches_query) {
                    computeBounds(); // more precise analysis or forward reachability
                    IntervalVisitor iv(_net, _place_bounds.get());
                    assert(_queries.size() == 1);
                    for (auto* q : _queries) {
                        PQL::Visitor::visit(iv, q);
                    }

#ifndef NDEBUG
                    PQL::EvaluationContext context(_parent->marking(), &_net);
                    auto r = PQL::evaluate(_queries[0], context);
                    if(iv.result() == IntervalVisitor::TRUE)
                        assert(r != PQL::Condition::RFALSE);
                    if(iv.result() == IntervalVisitor::FALSE)
                        assert(r != PQL::Condition::RTRUE);
#endif
                    if(!iv.stable())
                    {
                        skip();
                        return true;
                    }
                    // TODO, we could refine future enabled here!
                }
                for (auto t : _reach_actions)
                    addToStub(t);
            } else {
                for (auto t : _avoid_actions)
                    addToStub(t);
            }

            closure([this] {
                return !_added_unsafe;
            });

            PQL::EvaluationContext context(_parent->marking(), &_net);
            InterestingTransitionVisitor visitor(*this, false);
            assert(_queries.size() == 1);
            for (auto* q : _queries) {
                if (_is_safety)
                    visitor.negate();
                PetriEngine::PQL::evaluateAndSet(q, context);
                PQL::Visitor::visit(visitor, q);
            }

            if (_added_unsafe) {
                skip();
                return true;
            }

            closure([this] {
                return !_added_unsafe;
            });

            if (_added_unsafe) {
                skip();
                return true;
            }

            if(!reach_player) {
                if (!_added_enabled)
                    addToStub(_ordering.front());
            }

            closure([this] {
                return !_added_unsafe;
            });

            if (_added_unsafe) {
                skip();
                return true;
            }

            return true;
        }
    }
}