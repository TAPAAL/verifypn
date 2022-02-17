/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_MODELCHECKER_H
#define VERIFYPN_MODELCHECKER_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/SuccessorGeneration/ProductSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/ReachStubProductSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/ResumingSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"
#include "LTL/Structures/BitProductStateSet.h"
#include "LTL/SuccessorGeneration/ReachStubProductSuccessorGenerator.h"
#include "LTL/Structures/ProductStateFactory.h"
#include "PetriEngine/options.h"
#include "PetriEngine/Reducer.h"

#include <iomanip>
#include <algorithm>

namespace LTL {
    template<template <typename, typename...> typename ProductSucGen, typename SuccessorGen, typename... Spooler>
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet& net,
                     const PetriEngine::PQL::Condition_ptr &condition,
                     const Structures::BuchiAutomaton &buchi,
                     SuccessorGen& successorGen,
                     std::unique_ptr<Spooler> &&...spooler)
                : _net(net), _formula(condition), _successorGenerator(
                std::make_unique<ProductSucGen<SuccessorGen, Spooler...>>(net, buchi, successorGen,
                                                                          std::move(spooler)...)),
                  _factory(net, buchi, this->_successorGenerator->initial_buchi_state())
        { }

        /*void set_trace_level(TraceLevel level) {
            _traceLevel = level;
            if (_traceLevel != TraceLevel::None) {
                _maxTransName = 0;
                for (const auto &transname : _net->transitionNames()) {
                    _maxTransName = std::max(transname.size(), _maxTransName);
                }
            }
        }*/
        void set_utilize_weak(bool b) { _shortcircuitweak = b; }


        virtual bool is_satisfied() = 0;

        virtual ~ModelChecker() = default;

        //virtual void print_stats(std::ostream &os) = 0;

        [[nodiscard]] bool is_weak() const { return _is_weak; }

        size_t get_explored() { return _explored; }

    protected:
        size_t _explored = 0;
        size_t _expanded = 0;

        /*virtual void _printStats(std::ostream &os, const LTL::Structures::ProductStateSetInterface &stateSet)
        {
            std::cout << "STATS:\n"
                      << "\tdiscovered states: " << stateSet.discovered() << std::endl
                      << "\texplored states:   " << stats._explored << std::endl
                      << "\texpanded states:   " << stats._expanded << std::endl
                      << "\tmax tokens:        " << stateSet.max_tokens() << std::endl;
        }*/


        const PetriEngine::PetriNet& _net;
        PetriEngine::PQL::Condition_ptr _formula;
        std::unique_ptr<ProductSucGen<SuccessorGen, Spooler...>> _successorGenerator;
        LTL::Structures::ProductStateFactory _factory;

        size_t _discovered = 0;
        bool _shortcircuitweak;
        bool _weakskip = false;
        bool _is_weak = false;
        //size_t _maxTransName;

        /*
        static constexpr auto indent = "  ";
        static constexpr auto tokenIndent = "    ";

        void printLoop(std::ostream &os)
        {
            os << indent << "<loop/>\n";
        }

        std::ostream &
        printTransition(size_t transition, std::ostream &os, const LTL::Structures::ProductState* state = nullptr)
        {
            if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
                os << indent << "<deadlock/>";
                return os;
            }
            os << indent << "<transition id="
                // field width stuff obsolete without büchi state printing.
                << std::quoted(_net->transitionNames()[transition]);
            os << ">";
            if(_reducer) {
                _reducer->extraConsume(os, _net->transitionNames()[transition]);
            }
            os << std::endl;
            auto [fpre, lpre] = _net->preset(transition);
            for(; fpre < lpre; ++fpre) {
                if (fpre->inhibitor) {
                    assert(state == nullptr || state->marking()[fpre->place] < fpre->tokens);
                    continue;
                }
                for (size_t i = 0; i < fpre->tokens; ++i) {
                    assert(state == nullptr || state->marking()[fpre->place] >= fpre->tokens);
                    os << tokenIndent << R"(<token age="0" place=")" << _net->placeNames()[fpre->place] << "\"/>\n";
                }
            }
            os << indent << "</transition>\n";
            if(_reducer)
            {
                _reducer->postFire(os, _net->transitionNames()[transition]);
            }
           return os;
        }*/
    };
}

#endif //VERIFYPN_MODELCHECKER_H
