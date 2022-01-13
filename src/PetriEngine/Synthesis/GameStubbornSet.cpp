
#include "PetriEngine/Synthesis/GameStubbornSet.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"


namespace PetriEngine {
    namespace Synthesis {

        GameStubbornSet::GameStubbornSet(const PetriNet& net, PQL::Condition* predicate, bool is_safety)
        : StubbornSet(net, predicate), _is_safety(is_safety) {
            for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
                if (_net.controllable(t))
                    _reach_actions.emplace_back(t);
                else
                    _avoid_actions.emplace_back(t);
            }
            if (_is_safety)
                std::swap(_reach_actions, _avoid_actions);
            computeSafe();
        }

        void GameStubbornSet::addToStub(uint32_t t)
        {
            if(!_stubborn[t])
            {
                if(_enabled[t] && !_safe_actions[t])
                    _added_unsafe = true;
                else
                    StubbornSet::addToStub(t);
            }
        }

        uint32_t GameStubbornSet::next_env()
        {
            while(!_env_acts.empty())
            {
                if(_stubborn[_env_acts.back()])
                {
                    auto r = _env_acts.back();
                    _env_acts.pop_back();
                    return r;
                }
            }
            return std::numeric_limits<uint32_t>::max();
        }

        uint32_t GameStubbornSet::next_ctrl()
        {
            while(!_ctrl_acts.empty())
            {
                if(_stubborn[_ctrl_acts.back()])
                {
                    auto r = _ctrl_acts.back();
                    _ctrl_acts.pop_back();
                    return r;
                }
            }
            return std::numeric_limits<uint32_t>::max();
        }

        void GameStubbornSet::skip() {
            std::fill(_stubborn.get(), _stubborn.get() + _net.numberOfPlaces(), true);
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

        void GameStubbornSet::reset() {
            StubbornSet::reset();
            _ctrl_acts.clear();
            _env_acts.clear();
            _added_unsafe = false;
        }

        bool GameStubbornSet::prepare(const Structures::State *marking) {
            reset();
            _parent = marking;
            StubbornSet::constructEnabled([this](uint32_t t) {
                if (_net.controllable(t))
                    _ctrl_acts.push_back(t);
                else
                    _env_acts.push_back(t);
                return _env_acts.empty() || _ctrl_acts.empty();
            });

            if(_nenabled <= 1)
            {
                if(_nenabled == 1)
                    _stubborn[_ordering.back()];
                return true;
            }

            PQL::EvaluationContext context(_parent->marking(), &_net);
            InterestingTransitionVisitor visitor(*this, false);
            for(auto* q : _queries)
            {
                q->evalAndSet(context);
                q->visit(visitor);
            }

            if(_added_unsafe) { skip(); return true; }

            closure([this]{ return !_added_unsafe;});
            if(_added_unsafe) { skip(); return true; }

            const bool reach_player = _is_safety ? _ctrl_acts.empty() : _env_acts.empty();

            if (!reach_player) {
                // TODO; approximate forward reach to avoid accidential
                // acceptance
                for (auto t : _reach_actions)
                    addToStub(t);
                return false;
            } else {
                for (auto t : _avoid_actions)
                    addToStub(t);
            }
            closure([this]{ return !_added_unsafe;});
            if(_added_unsafe) { skip(); return true; }

            return true;
        }
    }
}