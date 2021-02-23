#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"
 
namespace PetriEngine {
    namespace Colored {
        class PartitionBuilder {
            public:
                PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap);
                ~PartitionBuilder() {}

                void initPartition();
                void refinePartition();
            private:
                std::vector<Transition> *_transitions;
                std::unordered_map<uint32_t,std::vector<uint32_t>> *_placePostTransitionMap;
                std::vector<Place> *_places;
                std::unordered_map<uint32_t, EquivalenceVec> partition;


        };
    }
}