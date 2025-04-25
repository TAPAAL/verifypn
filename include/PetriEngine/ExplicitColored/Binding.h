#ifndef BINDING_H
#define BINDING_H

#include "AtomicTypes.h"
#include <map>
#include <limits>

namespace PetriEngine::ExplicitColored {
    struct Binding
    {
        Binding() = default;
        explicit Binding(std::map<Variable_t, Color_t> map)
            : _values(std::move(map)) { };

        [[nodiscard]] Color_t getValue(const Variable_t v) const{
            if (const auto ret = _values.find(v); ret != _values.end()) {
                return ret->second;
            }
            return std::numeric_limits<Color_t>::max();
        }

        void setValue(const Variable_t v, const Color_t color) {
            _values.insert_or_assign(v, color);
        }

        Binding& operator=(std::map<Variable_t, Color_t>&& map) {
            this->_values = std::move(map);
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const Binding& binding) {
            out << "[";
            for (const auto&[var, val] : binding._values) {
                out << var << "=" << val << ",";
            }
            out << "]";
            return out;
        }
    private:
        std::map<Variable_t, Color_t> _values;
    };
}

#endif
