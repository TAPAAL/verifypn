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

#include <vector>
#include <unordered_map>

#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"

namespace PetriEngine {

    typedef std::unordered_map<std::string, Colored::ColorType*> ColorTypeMap;

    class NaiveBindingGenerator {
    public:
        class Iterator {
        private:
            NaiveBindingGenerator* _generator;
            
        public:
            Iterator(NaiveBindingGenerator* generator);
            
            bool operator==(Iterator& other);
            bool operator!=(Iterator& other);
            Iterator& operator++();
            const Colored::ExpressionContext::BindingMap operator++(int);
            Colored::ExpressionContext::BindingMap& operator*();
        };
    private:
        Colored::GuardExpression_ptr _expr;
        Colored::ExpressionContext::BindingMap _bindings;
        ColorTypeMap& _colorTypes;
        
        bool eval();
        
    public:
        NaiveBindingGenerator(Colored::Transition& transition,
                ColorTypeMap& colorTypes);

        Colored::ExpressionContext::BindingMap& nextBinding();
        Colored::ExpressionContext::BindingMap& currentBinding();
        bool isInitial() const;
        Iterator begin();
        Iterator end();
    };


    class FixpointBindingGenerator {
    public:
        class Iterator {
        private:
            FixpointBindingGenerator* _generator;
                        
        public:
            Iterator(FixpointBindingGenerator* generator);
            
            bool operator==(Iterator& other);
            bool operator!=(Iterator& other);
            Iterator& operator++();
            const Colored::ExpressionContext::BindingMap operator++(int);
            Colored::ExpressionContext::BindingMap& operator*();
        };
    private:
        Colored::GuardExpression_ptr _expr;
        Colored::ExpressionContext::BindingMap _bindings;
        const ColorTypeMap& _colorTypes;
        Colored::Transition *_transition;
        bool _isDone;
        bool _noValidBindings;
        uint32_t _nextIndex = 0;
        
        bool eval();
        
    public:
        FixpointBindingGenerator(Colored::Transition *transition,
                const ColorTypeMap& colorTypes);

        FixpointBindingGenerator(const FixpointBindingGenerator& ) = default;
        
        FixpointBindingGenerator operator= (const FixpointBindingGenerator& b) {
            return FixpointBindingGenerator(b);
        }

        Colored::ExpressionContext::BindingMap& nextBinding();
        Colored::ExpressionContext::BindingMap& currentBinding();
        bool isInitial() const;
        Iterator begin();
        Iterator end();
    };    
}