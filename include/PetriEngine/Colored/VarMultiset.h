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

        typedef std::vector<const Variable *> VarTuple;
        typedef std::vector<std::pair<VarTuple, uint32_t>> Internal;

        Internal _set;
        std::vector<const ColorType *> _types;
    public:
        VarMultiset() : _set(), _types() {};

        VarMultiset(std::vector<const ColorType *> &types) : _set(), _types(types) {};

        VarMultiset(const VarTuple &vars, uint32_t multiplicity);

        VarMultiset(const VarMultiset &) = default;

        VarMultiset(VarMultiset &&) = default;

        ~VarMultiset() = default;

        Iterator begin() const;

        Iterator end() const;

        VarMultiset &operator=(const VarMultiset &) = default;

        VarMultiset &operator=(VarMultiset &&) = default;

        bool operator==(const VarMultiset &other) const;

        VarMultiset operator+(const VarMultiset &other) const;

        VarMultiset operator-(const VarMultiset &other) const;

        VarMultiset operator*(uint32_t scalar) const;

        void operator+=(const VarMultiset &other);

        void operator-=(const VarMultiset &other);

        void operator*=(uint32_t scalar);

        uint32_t operator[](const VarTuple &vt) const;

        uint32_t &operator[](const VarTuple &vt);

        bool isSubsetOf(const VarMultiset &other) const;

        bool isSubsetOrEqTo(const VarMultiset &other) const;

        /** Returns the greatest number k, such that `this * k subseteq other`, but at most uint32_t::max */
        uint32_t numberOfTimesThisFitsInto(const VarMultiset &other) const;

        /** Returns the smallest number k, such that `other subseteq this * k`, if possible */
        std::optional<double> scaleRequiredToCover(const VarMultiset &other) const;

        bool divides(const VarMultiset &other) const;

        bool empty() const {
            return size() == 0;
        };

        size_t size() const;

        size_t distinctSize() const;

        size_t tupleSize() const {
            return _types.size();
        }

        std::string toString() const;

    private:

        bool matchesType(const VarTuple &vt) const;

        static std::vector<const ColorType *> inferTypes(const VarTuple &vt);

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

            std::pair<const VarTuple, const uint32_t &> operator*();
        };
    };
}

#endif //VERIFYPN_VARMULTISET_H
