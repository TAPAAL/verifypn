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

        Colored::ExpressionContext context {_bindings, _colorTypes};
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
}