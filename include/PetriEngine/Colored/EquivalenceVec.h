#ifndef EQUIVALENCEVEC_H
#define EQUIVALENCEVEC_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"
#include "EquivalenceClass.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceVec{
            public:
                void applyPartition(Colored::ArcIntervals& arcInterval) const;
                void mergeEqClasses();
                void applyPartition(std::vector<uint32_t> &colorIds) const;

                bool isDiagonal() const{
                    return _diagonal;
                }

                void setDiagonal(bool diagonal) {
                    _diagonal = diagonal;
                }

                const std::vector<EquivalenceClass> & getEquivalenceClasses() const{
                    return _equivalenceClasses;
                }

                std::vector<EquivalenceClass> & getMutEquivalenceClasses() {
                    return _equivalenceClasses;
                }

                const std::vector<bool> & getDiagonalTuplePositions() const{
                    return _diagonalTuplePositions;
                }

                std::vector<bool> & getMutDiagonalTuplePositions(){
                    return _diagonalTuplePositions;
                }

                void setDiagonalTuplePosition(uint32_t position, bool value){
                    _diagonalTuplePositions[position] = value;
                }

                void setDiagonalTuplePositions(const std::vector<bool> &diagonalPositions){
                    _diagonalTuplePositions = diagonalPositions;
                }

                std::unordered_map<const Colored::Color *, EquivalenceClass *> &getMutColorEqClassMap(){
                    return _colorEQClassMap;
                }

                const std::unordered_map<const Colored::Color *, EquivalenceClass *> &getColorEqClassMap() const{
                    return _colorEQClassMap;
                }

            private:
                std::vector<EquivalenceClass> _equivalenceClasses;
                std::unordered_map<const Colored::Color *, EquivalenceClass *> _colorEQClassMap;
                std::vector<bool> _diagonalTuplePositions;
                bool _diagonal = false;
        };
    }
}

#endif /* EQUIVALENCEVEC_H */