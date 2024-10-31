#ifndef BINDING_H
#define BINDING_H

#include "AtomicTypes.h"
#include <map>
#include <string>
#include <limits>

namespace PetriEngine {
    namespace ExplicitColored {
        struct Binding
        {
            Binding() = default;
            explicit Binding(std::map<Variable_t, Color_t> map)
                : _values(std::move(map)) { };

            Color_t getValue(const Variable_t v) const{
                if (auto ret = _values.find(v); ret != _values.end()) {
                    return ret->second;
                } else {
                    return std::numeric_limits<Color_t>::max();
                }
            }

            void setValue(const Variable_t v, const Color_t color) {
                _values.insert(std::make_pair(v, color));
            }
        private:
            std::map<Variable_t, Color_t> _values;
        };
    }
}

#endif