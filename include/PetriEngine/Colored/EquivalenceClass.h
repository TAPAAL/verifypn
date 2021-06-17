#ifndef EQUIVALENCECLASS_H
#define EQUIVALENCECLASS_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceClass {
            public:
                EquivalenceClass();
                EquivalenceClass(const ColorType *colorType);
                EquivalenceClass(const ColorType *colorType, const intervalTuple_t colorIntervals);
                ~EquivalenceClass() {}
                std::string toString() const{
                    return _colorIntervals.toString();
                }

                bool isEmpty() const{
                    if(_colorIntervals.size() < 1 || _colorIntervals.front().size() < 1){
                        return true;
                    } 
                    return false;
                }

                bool containsColor(const std::vector<uint32_t> &ids, const std::vector<bool> &diagonalPositions) const;

                size_t size() const;

                EquivalenceClass intersect(const EquivalenceClass &other) const;

                EquivalenceClass subtract(const EquivalenceClass &other, const std::vector<bool> &diagonalPositions) const;

                static uint32_t idCounter;
                uint32_t _id;
                const ColorType *_colorType;
                intervalTuple_t _colorIntervals;

            private:

            
        };
    }
}

#endif /* EQUIVALENCECLASS_H */