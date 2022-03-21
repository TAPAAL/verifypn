/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_VARMULTISET_H
#define VERIFYPN_VARMULTISET_H

#include <vector>
#include <cstdint>
#include "Colors.h"

namespace PetriEngine::Colored {
    class VarMultiset {
    private:
        typedef std::vector<std::pair<const Variable*, uint32_t>> Internal;

        Internal _set;
        const ColorType* _type;
    public:
        VarMultiset() : _set(), _type(nullptr) {};
        VarMultiset(const ColorType* type) : _set(), _type(type) {};
        VarMultiset(const VarMultiset&) = default;
        VarMultiset(VarMultiset&&) = default;
        VarMultiset(const Variable*, uint32_t);
        ~VarMultiset() = default;

        VarMultiset& operator=(const VarMultiset&) = default;
        VarMultiset& operator=(VarMultiset&&) = default;

        VarMultiset operator+ (const VarMultiset& other) const;
        VarMultiset operator- (const VarMultiset& other) const;
        VarMultiset operator* (uint32_t scalar) const;
        void operator+= (const VarMultiset& other);
        void operator-= (const VarMultiset& other);
        void operator*= (uint32_t scalar);
        uint32_t operator[] (const Variable* color) const;
        uint32_t& operator[] (const Variable* color);

        bool empty() const {
            return size() == 0;
        };

        size_t size() const;

        size_t distinctSize() const {
            return _set.size();
        }
    };
}

#endif //VERIFYPN_VARMULTISET_H
