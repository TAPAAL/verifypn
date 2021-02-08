#ifndef INTERVALGENERATOR_H
#define INTERVALGENERATOR_H

#include "ColoredNetStructures.h"

namespace PetriEngine {
    class IntervalGenerator {
        public:
            IntervalGenerator();
            virtual ~IntervalGenerator();
            bool getVarIntervals(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMaps, std::unordered_map<uint32_t, Colored::ArcIntervals> placeArcIntervals);
        private:
            std::vector<Colored::interval_t> getIntervalsFromInterval(Colored::interval_t *interval, uint32_t varPosition, int32_t varModifier, std::vector<Colored::ColorType*> varColorTypes);
            void getArcVarIntervals(Colored::intervalTuple_t& varIntervals, std::unordered_map<uint32_t, int32_t> modIndexMap, Colored::interval_t *interval, std::vector<Colored::ColorType*> varColorTypes);
        
    };    
}

#endif /* INTERVALGENERATOR_H */