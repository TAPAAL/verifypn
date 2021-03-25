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
#include "LTL/SuccessorGeneration/ResumingSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"
#include "LTL/Structures/BitProductStateSet.h"
#include "PetriEngine/options.h"
#include <iomanip>
#include <algorithm>

namespace LTL {
    template<typename SuccessorGen>
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet &net,
                     const PetriEngine::PQL::Condition_ptr &condition,
                     const Structures::BuchiAutomaton &buchi,
                     SuccessorGen &&successorGen,
                     const TraceLevel level = TraceLevel::Transitions,
                     bool shortcircuitweak = true)
                : net(net), formula(condition), traceLevel(level), shortcircuitweak(shortcircuitweak)
        {
            successorGenerator = std::make_unique<ProductSuccessorGenerator<SuccessorGen>>(net, buchi,
                                                                                           std::move(successorGen));
            if (level != TraceLevel::None) {
                maxTransName = 0;
                for (auto transname : net.transitionNames()) {
                    maxTransName = std::max(transname.size(), maxTransName);
                }
            }
        }

        virtual bool isSatisfied() = 0;

        virtual ~ModelChecker() = default;

        virtual void printStats(std::ostream &os) = 0;

        [[nodiscard]] bool isweak() const { return is_weak; }

        size_t get_explored() { return stats.explored; }

        void output_buchi(const std::string &file, BuchiOutType type) { successorGenerator->output_buchi(file, type); }

    protected:
        struct stats_t {
            size_t explored = 0, expanded = 0;
        };

        stats_t stats;

        virtual void _printStats(std::ostream &os, const LTL::Structures::ProductStateSetInterface &stateSet)
        {
            std::cout << "STATS:\n"
                      << "\tdiscovered states: " << stateSet.discovered() << std::endl
                      << "\texplored states:   " << stats.explored << std::endl
                      << "\texpanded states:   " << stats.expanded << std::endl
                      << "\tmax tokens:        " << stateSet.max_tokens() << std::endl;
        }

        std::unique_ptr<ProductSuccessorGenerator<SuccessorGen>> successorGenerator;

        const PetriEngine::PetriNet &net;
        PetriEngine::PQL::Condition_ptr formula;
        TraceLevel traceLevel;

        size_t _discovered = 0;
        const bool shortcircuitweak;
        bool weakskip = false;
        bool is_weak = false;
        size_t maxTransName;

        static constexpr auto indent = "  ";
        static constexpr auto tokenIndent = "    ";

        void printLoop(std::ostream &os)
        {
            os << indent << "<loop/>\n";
        }

        std::ostream &
        printTransition(size_t transition, LTL::Structures::ProductState &state, std::ostream &os)
        {
            if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
                os << indent << "<deadlock buchi=\"" << state.getBuchiState() << "\"/>";
                return os;
            }
            std::string tname = net.transitionNames()[transition];
            if (traceLevel == TraceLevel::Full) {
                os << indent << "<transition id=\"" << tname << "\">\n";
                for (size_t i = 0; i < net.numberOfPlaces(); ++i) {
                    for (size_t j = 0; j < state.marking()[i]; ++j) {
                        os << tokenIndent << R"(<token age="0" place=")" << net.placeNames()[i] << "\"/>\n";
                    }
                }
#ifndef NDEBUG
                os << '\n' << tokenIndent << "<buchi state=\"" << state.getBuchiState() << "\"/>\n";
#endif
                os << indent << "</transition>";
            } else {
                os << indent << "<transition id="
                   << std::setw(maxTransName + 1 + 2) << std::left
                   << std::quoted(tname) << " " << "buchisucc=\""
                   << state.getBuchiState() << "\"/>";
            }
            return os;
        }

    };

    extern template
    class ModelChecker<LTL::ResumingSuccessorGenerator>;

    extern template
    class ModelChecker<LTL::SpoolingSuccessorGenerator>;
}

#endif //VERIFYPN_MODELCHECKER_H
