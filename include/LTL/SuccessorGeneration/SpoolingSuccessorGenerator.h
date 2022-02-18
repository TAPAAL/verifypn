/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H
#define VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H

#include "LTL/Stubborn/AutomatonStubbornSet.h"
#include "LTL/Structures/ProductState.h"
#include "PetriEngine/Structures/SuccessorQueue.h"
#include "LTL/SuccessorGeneration/DistanceHeuristic.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "LTL/SuccessorGeneration/Heuristics.h"

namespace LTL {
    class SpoolingSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:
        SpoolingSuccessorGenerator(const PetriEngine::PetriNet& net, const PetriEngine::PQL::Condition_ptr &)
                : SuccessorGenerator(net), _transbuf(new uint32_t[net.numberOfTransitions()])
        {
            _statebuf.setMarking(new PetriEngine::MarkVal[net.numberOfPlaces() + 1], net.numberOfPlaces());
        }

        struct successor_info_t {
            SuccessorQueue<> _successors;
            size_t _buchi_state;
            size_t _last_state;
            size_t _transition;

            successor_info_t(size_t buchiState, size_t lastState) : _buchi_state(buchiState), _last_state(lastState) {}

            [[nodiscard]] bool has_prev_state() const
            {
                return _last_state != NoLastState;
            }

            size_t state() const {
                return _last_state;
            }

            size_t transition() const {
                return _transition;
            }

            [[nodiscard]] bool fresh() const { return _buchi_state == NoBuchiState && _last_state == NoLastState; }

            static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
            static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
        };

        void set_spooler(SuccessorSpooler& spooler)
        {
            _spooler = &spooler;
        }

        void set_heuristic(Heuristic *const heuristic)
        {
            _heuristic = heuristic;
        }

        [[nodiscard]] static successor_info_t initial_suc_info()
        {
            return successor_info_t{successor_info_t::NoBuchiState, successor_info_t::NoLastState};
        }

        bool prepare(const PetriEngine::Structures::State *state)
        {
            return PetriEngine::SuccessorGenerator::prepare(state);
        }

        bool next(PetriEngine::Structures::State &write)
        {
            return PetriEngine::SuccessorGenerator::next(write);
        }


        void prepare(const Structures::ProductState *state, successor_info_t &sucinfo)
        {
            assert(_spooler != nullptr);

            PetriEngine::SuccessorGenerator::prepare(state);
            if (sucinfo._successors == nullptr) {
                uint32_t tid;
#ifndef NDEBUG
                bool res =
#endif
                _spooler->prepare(state);
                if (!_heuristic || !_heuristic->has_heuristic(*state)) {
                    uint32_t nsuc = 0;
                    // generate list of transitions that generate a successor.
                    while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                        assert(tid <= _net.numberOfTransitions());
                        _transbuf[nsuc++] = tid;
                        assert(nsuc <= _net.numberOfTransitions());
                    }
                    sucinfo._successors = SuccessorQueue(_transbuf.get(), nsuc);
                    // assert((res && !sucinfo._successors.empty()) || !res); // this is suspected to be out of date
                } else {
                    // list of (transition, weight)
                    _heuristic->prepare(*state);
                    std::vector<std::pair<uint32_t, uint32_t>> weighted_tids;
                    while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                        assert(tid <= _net.numberOfTransitions());
                        SuccessorGenerator::_fire(_statebuf, tid);
                        _statebuf.set_buchi_state(state->get_buchi_state());
                        weighted_tids.emplace_back(tid, _heuristic->eval(_statebuf, tid));
                    }
                    // sort by least distance first.
                    std::sort(std::begin(weighted_tids), std::end(weighted_tids),
                              [](auto &l, auto &r) { return l.second < r.second; });
                    sucinfo._successors = SuccessorQueue(weighted_tids, [](auto &p) { return p.first; });
                }
            }
        }
        bool next(Structures::ProductState &state, successor_info_t &sucinfo)
        {
            assert(sucinfo._successors != nullptr);
            if (sucinfo._successors.empty()) {
                _last = std::numeric_limits<uint32_t>::max();
                return false;
            }
            _last = sucinfo._successors.front();
            sucinfo._transition = _last;
            sucinfo._successors.pop();
            SuccessorGenerator::_fire(state, _last);
            return true;
        }

        [[nodiscard]] uint32_t fired() const { return _last; }

        void generate_all(LTL::Structures::ProductState *parent, successor_info_t &sucinfo)
        {
            assert(_spooler != nullptr);
            assert(sucinfo._successors != nullptr);
            if (!_spooler->generateAll(parent)) return;
            assert(dynamic_cast<VisibleLTLStubbornSet*>(_spooler) != nullptr);

            uint32_t tid;
            if (!_heuristic) {
                uint32_t nsuc = 0;
                // generate list of transitions that generate a successor.
                auto [first, last] = sucinfo._successors.all_successors();
                for (; first < last; ++first) {
                    tid = *first;
                    // avoiding duplicates
                    if (!static_cast<VisibleLTLStubbornSet*>(_spooler)->stubborn()[tid]) {
                        _transbuf[nsuc++] = tid;
                    }
                }
                while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                    assert(tid <= _net.numberOfTransitions());
                    _transbuf[nsuc++] = tid;
                    assert(nsuc <= _net.numberOfTransitions());
                }
                sucinfo._successors.extend_to(_transbuf.get(), nsuc);

            } else {
                auto evaluate_heuristic = [&] (uint32_t tid) {
                    SuccessorGenerator::_fire(_statebuf, tid);
                    _statebuf.set_buchi_state(sucinfo._buchi_state);
                    return _heuristic->eval(_statebuf, tid);
                };

                // list of (transition, weight)
                std::vector<std::pair<uint32_t, uint32_t>> weighted_tids;
                // grab previous stubborn transitions
                auto [first, last] = sucinfo._successors.all_successors();
                for (; first < last; ++first) {
                    tid = *first;
                    if (!static_cast<VisibleLTLStubbornSet*>(_spooler)->stubborn()[tid]) {
                        weighted_tids.emplace_back(tid, evaluate_heuristic(tid));
                    }
                }

                // add new stubborn transitions
                while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                    assert(tid <= _net.numberOfTransitions());
                    weighted_tids.emplace_back(tid, evaluate_heuristic(tid));
                }
                // sort by least distance first.
                std::sort(std::begin(weighted_tids), std::end(weighted_tids),
                          [](auto &l, auto &r) { return l.second < r.second; });
                assert(weighted_tids.size() <= _net.numberOfTransitions());
                std::transform(std::begin(weighted_tids), std::end(weighted_tids),
                               _transbuf.get(),
                               [](auto &p) { return p.first; });
                sucinfo._successors.extend_to(_transbuf.get(), weighted_tids.size());
            }
        }

        void push() {
            // No transitions have been fired yet. We must be in the initial marking.
            if (!_heuristic || fired() == std::numeric_limits<uint32_t>::max()) return;
            _heuristic->push(fired());
        }

        void pop(const successor_info_t &sc) {
            if (_heuristic && sc._successors.has_consumed())
                _heuristic->pop(sc._successors.last_pop());
        }

    private:
        SuccessorSpooler* _spooler = nullptr;
        Heuristic* _heuristic = nullptr;

        uint32_t _last = std::numeric_limits<uint32_t>::max();
        std::unique_ptr<uint32_t[]> _transbuf;   /* buffer for enabled transitions, size is ntransitions. */
        LTL::Structures::ProductState _statebuf;
    };
}
#endif //VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H
