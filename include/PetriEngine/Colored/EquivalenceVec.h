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


                const std::vector<bool> & getDiagonalTuplePositions() const{
                    return _diagonalTuplePositions;
                }

                void push_back_Eqclass(const EquivalenceClass &Eqclass){
                    _equivalenceClasses.push_back(Eqclass);
                }

                void erase_Eqclass(uint32_t position){
                    _equivalenceClasses.erase(_equivalenceClasses.begin() + position);
                }

                void push_back_diagonalTuplePos(bool val){
                    _diagonalTuplePositions.push_back(val);
                }

                void addColorToEqClassMap(const Color *color);

                void setDiagonalTuplePosition(uint32_t position, bool value){
                    _diagonalTuplePositions[position] = value;
                }

                void setDiagonalTuplePositions(const std::vector<bool> &diagonalPositions){
                    _diagonalTuplePositions = diagonalPositions;
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