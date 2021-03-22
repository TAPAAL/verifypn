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

#include "PetriEngine/Colored/BindingGenerator.h"

namespace PetriEngine {

    FixpointBindingGenerator::Iterator::Iterator(FixpointBindingGenerator* generator)
            : _generator(generator)
    {
    }

    bool FixpointBindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool FixpointBindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    FixpointBindingGenerator::Iterator& FixpointBindingGenerator::Iterator::operator++() {
        if (_generator->_isDone) {
            _generator = nullptr;
        } else {
            _generator->nextBinding();
            if (_generator->_isDone) {
                _generator = nullptr;
            }
        }
        return *this;
    }

    const Colored::ExpressionContext::BindingMap FixpointBindingGenerator::Iterator::operator++(int) {
        auto prev = _generator->currentBinding();
        ++*this;
        return prev;
    }

    Colored::ExpressionContext::BindingMap& FixpointBindingGenerator::Iterator::operator*() {
        return _generator->currentBinding();
    }

    FixpointBindingGenerator::FixpointBindingGenerator(Colored::Transition& transition,
        ColorTypeMap& colorTypes)
    : _colorTypes(colorTypes), _transition(transition)
    {
        _isDone = false;
        _noValidBindings = false;
        _nextIndex = 0;
        _expr = _transition.guard;

        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (auto arc : _transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : _transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }       
        
        for (auto var : variables) {
            if(_transition.variableMaps.empty() || _transition.variableMaps[_nextIndex][var]._intervals.empty()){
                _noValidBindings = true;
                break;
            }
            auto color = var->colorType->getColor(_transition.variableMaps[_nextIndex][var].getFirst().getLowerIds());
            _bindings[var] = color;
        }
        
        if (!_noValidBindings && !eval())
            nextBinding();
    }


    bool FixpointBindingGenerator::eval() {
        if (_expr == nullptr)
            return true;

        Colored::ExpressionContext context {_bindings, _colorTypes};
        return _expr->eval(context);
    }

    Colored::ExpressionContext::BindingMap& FixpointBindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            bool next = true;

            for (auto& _binding : _bindings) {
                auto varInterval = _transition.variableMaps[_nextIndex][_binding.first];
                std::vector<uint32_t> colorIds;
                _binding.second->getTupleId(&colorIds);
                auto nextIntervalBinding = varInterval.isRangeEnd(colorIds);

                if (nextIntervalBinding.size() == 0){
                    _binding.second = &_binding.second->operator++();
                    next = false;
                    break;                    
                } else {
                    _binding.second = _binding.second->getColorType()->getColor(nextIntervalBinding.getLowerIds());
                    if(!nextIntervalBinding.equals(varInterval.getFirst())){
                        next = false;
                        break;
                    }              
                }
            }
            if(next){
                _nextIndex++;
                if(isInitial()){
                    _isDone = true;
                    break;
                }
                for(auto& _binding : _bindings){
                    _binding.second =  _binding.second->getColorType()->getColor(_transition.variableMaps[_nextIndex][_binding.first].getFirst().getLowerIds());
                }
            }
                 
            test = eval();
        
        }
        
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& FixpointBindingGenerator::currentBinding() {
        return _bindings;
    }

    bool FixpointBindingGenerator::isInitial() const{        
        return _nextIndex >= _transition.variableMaps.size();
    }

    FixpointBindingGenerator::Iterator FixpointBindingGenerator::begin() {
        if(_noValidBindings || _isDone){
            return {nullptr};
        }
        return {this};
    }

    FixpointBindingGenerator::Iterator FixpointBindingGenerator::end() {
        return {nullptr};
    }
}