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

#include "LTL/Algorithm/ModelChecker.h"

#include <utility>
#include <iomanip>

namespace LTL {
    ModelChecker::ModelChecker(const PetriEngine::PetriNet &net, PetriEngine::PQL::Condition_ptr condition,
                               const bool shortcircuitweak, TraceLevel level)
            : net(net), formula(condition), traceLevel(level), shortcircuitweak(shortcircuitweak)
    {

        successorGenerator = std::make_unique<ProductSuccessorGenerator>(net, condition);

        maxTransName = std::max_element(std::begin(net.transitionNames()), std::end(net.transitionNames()),
                                        [](auto &a, auto &b) { return a.size() < b.size(); })->size();

    }

    static constexpr auto indent = "  ";
    static constexpr auto tokenIndent = "    ";

    void ModelChecker::printLoop(std::ostream &os)
    {
        os << indent << "<loop/>\n";
    }

    std::ostream &
    ModelChecker::printTransition(size_t transition, LTL::Structures::ProductState &state, std::ostream &os)
    {
        if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
            os << indent << "<deadlock/>";
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
            os << indent << "<transition id=" << std::setw(maxTransName + 1) << std::quoted(tname) << "\tbuchisucc=\""
               << state.getBuchiState() << "\"/>";
        }
        return os;
    }

}

