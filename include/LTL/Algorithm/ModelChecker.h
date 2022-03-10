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

    class ModelChecker {
    public:

        ModelChecker(const PetriEngine::PetriNet& net,
                const PetriEngine::PQL::Condition_ptr &condition,
                const Structures::BuchiAutomaton &buchi)
        : _net(net), _formula(condition),
          _factory(net, buchi), _buchi(buchi) {
        }

        void set_tracing(bool tracing) { _build_trace = tracing; }

        void set_heuristic(Heuristic* heuristic) {
            _heuristic = heuristic;
        }

        void set_utilize_weak(bool b) {
            _shortcircuitweak = b;
        }

        virtual void set_partial_order(LTLPartialOrder) {}

        virtual bool check() = 0;

        virtual ~ModelChecker() = default;

        [[nodiscard]] bool is_weak() const {
            return _shortcircuitweak;
        }

        size_t get_explored() {
            return _explored;
        }

        virtual void print_stats(std::ostream&) const = 0;

        size_t loop_index() const {
            return _loop;
        }

        const std::vector<size_t>& trace() const {
            return _trace;
        }


        virtual LTLPartialOrder used_partial_order() const {
            return LTLPartialOrder::None;
        }


    protected:
        size_t _explored = 0;
        size_t _expanded = 0;

        virtual void print_stats(std::ostream &os, size_t discovered, size_t max_tokens) const {
            std::cout << "STATS:\n"
                    << "\tdiscovered states: " << discovered << std::endl
                    << "\texplored states:   " << _explored << std::endl
                    << "\texpanded states:   " << _expanded << std::endl
                    << "\tmax tokens:        " << max_tokens << std::endl;
        }

        const PetriEngine::PetriNet& _net;
        PetriEngine::PQL::Condition_ptr _formula;
        Structures::ProductStateFactory _factory;
        const Structures::BuchiAutomaton& _buchi;
        bool _shortcircuitweak;
        bool _build_trace = false;
        Heuristic* _heuristic = nullptr;
        size_t _loop = std::numeric_limits<size_t>::max();
        std::vector<size_t> _trace;
        bool _violation = false;
    };
}

#endif //VERIFYPN_MODELCHECKER_H
