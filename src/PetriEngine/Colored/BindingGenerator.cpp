#include "PetriEngine/Colored/BindingGenerator.h"

namespace PetriEngine {

    NaiveBindingGenerator::Iterator::Iterator(NaiveBindingGenerator* generator)
            : _generator(generator)
    {
    }

    bool NaiveBindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool NaiveBindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    NaiveBindingGenerator::Iterator& NaiveBindingGenerator::Iterator::operator++() {
        _generator->nextBinding();
        if (_generator->isInitial()) _generator = nullptr;
        return *this;
    }

    const Colored::ExpressionContext::BindingMap NaiveBindingGenerator::Iterator::operator++(int) {
        auto prev = _generator->currentBinding();
        ++*this;
        return prev;
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::Iterator::operator*() {
        return _generator->currentBinding();
    }

    NaiveBindingGenerator::NaiveBindingGenerator(Colored::Transition& transition,
            ColorTypeMap& colorTypes)
        : _colorTypes(colorTypes)
    {
        _expr = transition.guard;
        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (auto arc : transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto var : variables) {
            _bindings[var] = &var->colorType->operator[](0);
        }
        
        if (!eval())
            nextBinding();
    }

    bool NaiveBindingGenerator::eval() {
        if (_expr == nullptr)
            return true;
        Colored::EquivalenceVec placePartition;

        Colored::ExpressionContext context {_bindings, _colorTypes, placePartition};
        return _expr->eval(context);
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            for (auto& _binding : _bindings) {
                _binding.second = &_binding.second->operator++();
                if (_binding.second->getId() != 0) {
                    break;
                }
            }

            if (isInitial())
                break;

            test = eval();
        }
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::currentBinding() {
        return _bindings;
    }

    bool NaiveBindingGenerator::isInitial() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != 0) return false;
        }
        return true;
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::begin() {
        return {this};
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::end() {
        return {nullptr};
    }



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

        //combine varmaps
        // for(uint i = 1; i < _transition.variableMaps.size(); i++){
        //     for(auto varPair : transition.variableMaps[i]){
        //         for(auto interval : varPair.second._intervals){
        //             transition.variableMaps[0][varPair.first].addInterval(interval);
        //         }                
        //     }
        // }

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

        // if(_transition.name == "continueLoop"){
        //     std::cout << _transition.name << " varmap size " << _transition.variableMaps.size() << std::endl;
        //     for(auto varMap : _transition.variableMaps){
        //         std::cout << "Var set:" << std::endl;
        //         for(auto pair : varMap){
        //             std::cout << pair.first->name << "\t";
        //             for(auto interval : pair.second._intervals){
        //                 interval.print();
        //                 std::cout << " ";
        //             }
        //             std::cout << std::endl;
        //         }
        //     }
        // }
        
        
        
        
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
        // std::cout << "testing for " << _transition.name << std::endl;
        // for (auto test : _bindings){
        //     std::cout << "Binding '" << test.first->name << "\t" << test.second->getColorName() << "' in bindings." << std::endl;
        // }
        // std::cout << std::endl;
        if (_expr == nullptr)
            return true;

        Colored::EquivalenceVec placePartition;
        Colored::ExpressionContext context {_bindings, _colorTypes, placePartition};
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