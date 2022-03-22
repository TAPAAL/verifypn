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
        class Iterator;

        typedef std::vector<std::pair<const Variable *, uint32_t>> Internal;

        Internal _set;
        const ColorType *_type;
    public:
        VarMultiset() : _set(), _type(nullptr) {};

        VarMultiset(const ColorType *type) : _set(), _type(type) {};

        VarMultiset(const VarMultiset &) = default;

        VarMultiset(VarMultiset &&) = default;

        VarMultiset(const Variable *, uint32_t);

        ~VarMultiset() = default;

        Iterator begin() const;

        Iterator end() const;

        VarMultiset &operator=(const VarMultiset &) = default;

        VarMultiset &operator=(VarMultiset &&) = default;

        VarMultiset operator+(const VarMultiset &other) const;

        VarMultiset operator-(const VarMultiset &other) const;

        VarMultiset operator*(uint32_t scalar) const;

        void operator+=(const VarMultiset &other);

        void operator-=(const VarMultiset &other);

        void operator*=(uint32_t scalar);

        uint32_t operator[](const Variable *color) const;

        uint32_t &operator[](const Variable *color);

        bool isSubsetOf(const VarMultiset &other) const;

        bool empty() const {
            return size() == 0;
        };

        size_t size() const;

        size_t distinctSize() const {
            return _set.size();
        }

        std::string toString() const;

    private:
        class Iterator {
        private:
            const VarMultiset *_ms;
            size_t _index;

        public:
            Iterator(const VarMultiset *ms, size_t index)
                    : _ms(ms), _index(index) {}

            bool operator==(Iterator &other);

            bool operator!=(Iterator &other);

            Iterator &operator++();

            std::pair<const Variable *, const uint32_t &> operator++(int);

            std::pair<const Variable *, const uint32_t &> operator*();
        };
    };
}

#endif //VERIFYPN_VARMULTISET_H
