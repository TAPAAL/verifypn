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

#include "LTL/Structures/ProductState.h"
#include "PetriEngine/Structures/SuccessorQueue.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "LTL/SuccessorGeneration/Heuristics.h"

namespace LTL {
    class SpoolingSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:
        SpoolingSuccessorGenerator(const PetriEngine::PetriNet *net, const PetriEngine::PQL::Condition_ptr &)
                : SuccessorGenerator(*net), _transbuf(new uint32_t[net->numberOfTransitions()])
        {
            _statebuf.setMarking(new PetriEngine::MarkVal[net->numberOfPlaces() + 1], net->numberOfPlaces());
        }

        struct sucinfo {
            SuccessorQueue<> successors;
            size_t buchi_state;
            size_t last_state;

            sucinfo(size_t buchiState, size_t lastState) : buchi_state(buchiState), last_state(lastState) {}

            [[nodiscard]] inline bool has_prev_state() const
            {
                return last_state != NoLastState;
            }

            [[nodiscard]] bool fresh() const { return buchi_state == NoBuchiState && last_state == NoLastState; }

            static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
            static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
        };

        void setSpooler(std::unique_ptr<SuccessorSpooler> &&spooler)
        {
            _spooler.swap(spooler);
        }

        void setHeuristic(std::unique_ptr<Heuristic> &&heuristic)
        {
            _heuristic.swap(heuristic);
        }

        [[nodiscard]] static sucinfo initial_suc_info()
        {
            return sucinfo{sucinfo::NoBuchiState, sucinfo::NoLastState};
        }

        bool prepare(const PetriEngine::Structures::State *state)
        {
            return PetriEngine::SuccessorGenerator::prepare(state);
        }

        bool next(PetriEngine::Structures::State &write)
        {
            return PetriEngine::SuccessorGenerator::next(write);
        }


        void prepare(const Structures::ProductState *state, sucinfo &sucinfo)
        {
            assert(_spooler != nullptr);

            PetriEngine::SuccessorGenerator::prepare(state);
            if (sucinfo.successors == nullptr) {
                uint32_t tid;
                _spooler->prepare(state);
                if (!_heuristic || !_heuristic->has_heuristic(*state)) {
                    uint32_t nsuc = 0;
                    // generate list of transitions that generate a successor.
                    while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                        assert(tid <= _net.numberOfTransitions());
                        _transbuf[nsuc++] = tid;
                        assert(nsuc <= _net.numberOfTransitions());
                    }
                    sucinfo.successors = SuccessorQueue(_transbuf.get(), nsuc);

                } else {
                    // list of (transition, weight)
                    std::vector<std::pair<uint32_t, uint32_t>> weighted_tids;
                    while ((tid = _spooler->next()) != SuccessorSpooler::NoTransition) {
                        assert(tid <= _net.numberOfTransitions());
                        SuccessorGenerator::_fire(_statebuf, tid);
                        _statebuf.setBuchiState(state->getBuchiState());
                        weighted_tids.emplace_back(tid, _heuristic->eval(_statebuf, tid));
                    }
                    // sort by least distance first.
                    std::sort(std::begin(weighted_tids), std::end(weighted_tids),
                              [](auto &l, auto &r) { return l.second < r.second; });
                    sucinfo.successors = SuccessorQueue(weighted_tids.data(), weighted_tids.size(),
                                                        [](auto &p) { return p.first; });
                }
            }
        }

        bool next(Structures::ProductState &state, sucinfo &sucinfo)
        {
            assert(sucinfo.successors != nullptr);
            if (sucinfo.successors.empty()) {
                _last = std::numeric_limits<uint32_t>::max();
                return false;
            }
            _last = sucinfo.successors.front();
            sucinfo.successors.pop();
            SuccessorGenerator::_fire(state, _last);
            return true;
        }

        [[nodiscard]] uint32_t fired() const { return _last; }

        void _fire(Structures::ProductState &parent, Structures::ProductState &write, uint32_t tid)
        {
            PetriEngine::SuccessorGenerator::_fire(write, tid);
            write.setBuchiState(parent.getBuchiState());
        }


    private:
        std::unique_ptr<SuccessorSpooler> _spooler = nullptr;
        std::unique_ptr<Heuristic> _heuristic = nullptr;

        uint32_t _last;
        std::unique_ptr<uint32_t[]> _transbuf;   /* buffer for enabled transitions, size is ntransitions. */
        LTL::Structures::ProductState _statebuf;
    };
}
#endif //VERIFYPN_SPOOLINGSUCCESSORGENERATOR_H
