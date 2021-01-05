#include <vector>
#include <unordered_map>

#include "ColoredNetStructures.h"

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
  
}