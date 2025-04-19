/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLOREDPETRINETBUILDER_H
#define COLOREDPETRINETBUILDER_H

#include <vector>
#include <unordered_map>
#include <istream>

#include "../AbstractPetriNetBuilder.h"
#include "../PetriNetBuilder.h"

namespace PetriEngine {
    namespace Colored { namespace Reduction {
        class ColoredReducer;
    } }
    namespace Colored {
        class PnmlWriter;
    }

    class ColoredPetriNetBuilder : public AbstractPetriNetBuilder {
    public:
        friend class Colored::Reduction::ColoredReducer;
        friend class Colored::PnmlWriter;

        ColoredPetriNetBuilder(shared_string_set& string_set);
        ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig);
        virtual ~ColoredPetriNetBuilder();

        void addPlace(const std::string& name,
                uint32_t tokens,
                double x,
                double y) override ;
        void addPlace(const std::string& name,
                const Colored::ColorType* type,
                Colored::Multiset&& tokens,
                double x,
                double y) override;
        void addTransition(const std::string& name,
                int32_t player,
                double x,
                double y) override;
        void addTransition(const std::string& name,
                const Colored::GuardExpression_ptr& guard,
                int32_t player,
                double x,
                double y) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                uint32_t weight) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr &expr,
                uint32_t inhib_weight) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                uint32_t weight) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                const Colored::ArcExpression_ptr& expr) override;
        void addColorType(const std::string& id,
                const Colored::ColorType* type) override;
        void addToColorType(Colored::ProductType* colorType,
            const Colored::ColorType* newConstituent) override;

        void sort() override;

        uint32_t getPlaceCount() const {
            return _places.size();
        }

        uint32_t getTransitionCount() const {
            return _transitions.size();
        }

        uint32_t getArcCount() const {
            uint32_t sum = 0;
            for (auto& t : _transitions) {
                sum += t.input_arcs.size();
                sum += t.output_arcs.size();
            }
            return sum;
        }

        uint32_t unskippedPlacesCount() const {
            uint32_t count = 0;
            for (auto &p : _places) {
                if (!p.skipped) count++;
            }
            return count;
        }

        uint32_t unskippedTransitionsCount() const {
            uint32_t count = 0;
            for (auto &t : _transitions) {
                if (!t.skipped) count++;
            }
            return count;
        }

        const std::vector<Colored::Place>& places() const {
            return _places;
        }

        const std::vector<Colored::Transition>& transitions() const {
            return _transitions;
        }

        auto& colored_placenames() const {
            return _placenames;
        }

        auto& colored_transitionnames() const {
            return _transitionnames;
        }

        auto& inhibitors() const {
            return _inhibitorArcs;
        }

        auto& colors() const {
            return _colors;
        }

        PetriNetBuilder pt_builder() const {
            return _ptBuilder;
        }

        shared_string_set& string_set() const {
            return _string_set;
        }

    private:
        shared_name_index_map _placenames;
        shared_name_index_map _transitionnames;

        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::Arc> _inhibitorArcs;
        std::vector<const Colored::Variable *> _variables;

        Colored::ColorTypeMap _colors;
        PetriNetBuilder _ptBuilder;
        shared_string_set& _string_set;

        void addArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool input, uint32_t inhib_weight);

        void addVariable(const Colored::Variable* variable) override;
    };

    enum ExpressionType {
        None,
        Pred,
        Succ
    };


}

#endif /* COLOREDPETRINETBUILDER_H */
