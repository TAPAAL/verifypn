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
                _values.insert_or_assign(v, color);
            }

            friend std::ostream& operator<<(std::ostream& out, const Binding& binding) {
                out << "[";
                for (const auto& val : binding._values) {
                    out << val.first << "=" << val.second << ",";
                }
                out << "]";
                return out;
            }
        private:
            std::map<Variable_t, Color_t> _values;
        };
    }
}

#endif