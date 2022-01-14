
#include "PetriEngine/Synthesis/GameStubbornSet.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

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
            for(size_t t = 0; t < _net.numberOfTransitions(); ++t)
            {
                auto [finv, linv] = _net.preset(t);
                for(;finv < linv; ++finv)
                    if(finv->inhibitor)
                        _inhibiting_place[finv->place] = true;
            }
            predicate->visit(_in_query);
        }

        void GameStubbornSet::addToStub(uint32_t t) {
            if (!_stubborn[t]) {
                if(_enabled[t])
                    _added_enabled = true;
                if (_enabled[t] && !_safe_actions[t])
                    _added_unsafe = true;
                else
                {
                    if(_env_acts.empty() == _net.controllable(t) &&
                        !_future_enabled[t])
                        return;
                    StubbornSet::addToStub(t);
                }
            }
        }

        uint32_t GameStubbornSet::next_env() {
            while (!_env_acts.empty()) {
                auto r = _env_acts.back();
                _env_acts.pop_back();
                if (_stubborn[r]) return r;
            }
            return std::numeric_limits<uint32_t>::max();
        }

        uint32_t GameStubbornSet::next_ctrl() {
            while (!_ctrl_acts.empty()) {
                auto r = _ctrl_acts.back();
                _ctrl_acts.pop_back();
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
            if(!_ctrl_acts.empty() &&
               !_env_acts.empty())
            {
                skip();
                return true;
            }

            if (_nenabled <= 1) {
                if (_nenabled == 1)
                    _stubborn[_ordering.front()] = true;
                return true;
            }

            const auto touches_query = approximateFuture(!_ctrl_acts.empty());
            PQL::EvaluationContext context(_parent->marking(), &_net);
            InterestingTransitionVisitor visitor(*this, false);
            assert(_queries.size() == 1);
            for (auto* q : _queries) {
                if(_is_safety)
                    visitor.negate();
                q->evalAndSet(context);
                q->visit(visitor);
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

            const bool reach_player = _is_safety ? _ctrl_acts.empty() : _env_acts.empty();

            if (!reach_player) {
                // TODO; approximate forward reach to avoid accidential
                // acceptance
                if(touches_query)
                {
                    skip();
                    return true;
                }
                for (auto t : _reach_actions)
                    addToStub(t);
                closure([this] {
                    return !_added_unsafe;
                });
                if(!_added_enabled)
                    addToStub(_ordering.front());
            } else {
                for (auto t : _avoid_actions)
                    addToStub(t);
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