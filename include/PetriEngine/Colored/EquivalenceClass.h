#ifndef PARTITION_H
#define PARTITION_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceClass {
            public:
                EquivalenceClass();
                EquivalenceClass(ColorType *colorType);
                EquivalenceClass(ColorType *colorType, intervalTuple_t colorIntervals);
                ~EquivalenceClass() {}
                std::string toString(){
                    return _colorIntervals.toString();
                }

                bool isEmpty(){
                    if(_colorIntervals.size() < 1 || _colorIntervals.getFirst().size() < 1){
                        return true;
                    } 
                    return false;
                }

                bool containsColor(std::vector<uint32_t> ids);

                size_t size();

                EquivalenceClass intersect(EquivalenceClass other);

                EquivalenceClass subtract(EquivalenceClass other, bool print);

                static uint32_t idCounter;
                uint32_t _id;
                ColorType *_colorType;
                intervalTuple_t _colorIntervals;

            private:

            
        };

        struct EquivalenceVec{
            std::vector<EquivalenceClass> _equivalenceClasses;
            std::unordered_map<const Colored::Color *, EquivalenceClass *> colorEQClassMap;
            bool diagonal = false;

            void applyPartition(Colored::ArcIntervals& arcInterval){
                if(diagonal || _equivalenceClasses.size() == _equivalenceClasses.back()._colorType->size()){
                    diagonal = true;
                    return;
                }
                std::vector<Colored::intervalTuple_t> newTupleVec;
                for(auto intervalTuple : arcInterval._intervalTupleVec){
                    intervalTuple.combineNeighbours();
                    intervalTuple_t newIntervalTuple;
                    for(auto interval : intervalTuple._intervals){
                        for(auto EQClass : _equivalenceClasses){
                            for(auto EQinterval : EQClass._colorIntervals._intervals){
                                auto overlap = interval.getOverlap(EQinterval);
                                if(overlap.isSound()){
                                    newIntervalTuple.addInterval(EQinterval.getSingleColorInterval());
                                    continue;
                                }
                            }
                        }
                    }
                   newTupleVec.push_back(std::move(newIntervalTuple));
                }
                arcInterval._intervalTupleVec = std::move(newTupleVec);               
            }
        };

    }
}

#endif /* PARTITION_H */