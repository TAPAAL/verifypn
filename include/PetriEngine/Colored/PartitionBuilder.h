#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"
#include "IntervalGenerator.h"
 
namespace PetriEngine {
    namespace Colored {
        class PartitionBuilder {
            public:
                PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap);
                ~PartitionBuilder() {}

                //void initPartition();
                void partitionNet();
                void refinePartition();
                void printPartion();

                std::unordered_map<uint32_t, EquivalenceVec> getPartition(){
                    return _partition;
                }
            private:
                std::vector<Transition> *_transitions;
                std::unordered_map<uint32_t,std::vector<uint32_t>> *_placePostTransitionMap;
                std::unordered_map<uint32_t,std::vector<uint32_t>> *_placePreTransitionMap;
                std::unordered_map<uint32_t,bool> _inQueue;
                std::vector<Place> *_places;
                std::unordered_map<uint32_t, EquivalenceVec> _partition;
                PetriEngine::IntervalGenerator intervalGenerator;
                std::vector<uint32_t> _placeQueue;

                bool splitPartition(EquivalenceVec equivalenceVec, uint32_t placeId);

                void handleTransition(uint32_t transitionId, uint32_t postPlaceId);
                void handleTransition(Transition *transitionId, uint32_t postPlaceId, Arc *postArc);

                void handleLeafTransitions();
                void assignColorMap();

                std::vector<std::unordered_map<const Variable *, intervalTuple_t>> prepareVariables(
                            std::unordered_map<const Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap, 
                            EquivalenceClass *eqClass , Arc *postArc, uint32_t placeId);

                bool findOverlap(EquivalenceVec equivalenceVec1, EquivalenceVec equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection);
        };
    }
}