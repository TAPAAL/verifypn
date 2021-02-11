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
#include "LTL/ProductSuccessorGenerator.h"
#include "LTL/Algorithm/ProductPrinter.h"

namespace LTL {
    template<typename SuccessorGen>
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet &net, const PetriEngine::PQL::Condition_ptr &condition,
                     const SuccessorGen &successorGen,
                     bool shortcircuitweak = true)
                : net(net), formula(condition), shortcircuitweak(shortcircuitweak) {
            successorGenerator = std::make_unique<ProductSuccessorGenerator<SuccessorGen>>(net, condition,
                                                                                           successorGen);
        }

        virtual bool isSatisfied() = 0;

        virtual ~ModelChecker() = default;

        virtual void printStats(std::ostream &os) = 0;

        [[nodiscard]] bool isweak() const { return is_weak; }

    protected:
        struct stats_t {
            size_t explored = 0, expanded = 0;
        };

        stats_t stats;
        virtual void _printStats(ostream &os, const PetriEngine::Structures::StateSet &stateSet) {
            std::cout   << "STATS:\n"
                        << "\tdiscovered states: " << stateSet.discovered() << std::endl
                        << "\texplored states:   " << stats.explored << std::endl
                        << "\texpanded states:   " << stats.expanded << std::endl
                        << "\tmax tokens:        " << stateSet.maxTokens() << std::endl;
        }

        std::unique_ptr<ProductSuccessorGenerator<SuccessorGen>> successorGenerator;
        const PetriEngine::PetriNet &net;
        PetriEngine::PQL::Condition_ptr formula;

        size_t _discovered = 0;
        const bool shortcircuitweak;
        bool weakskip = false;
        bool is_weak = false;


    };
}

#endif //VERIFYPN_MODELCHECKER_H
