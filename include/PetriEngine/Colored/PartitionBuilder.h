#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"
#include "IntervalGenerator.h"
 
namespace PetriEngine {
    namespace Colored {
        class PartitionBuilder {

            private:
                std::vector<Transition> *_transitions;
                std::unordered_map<uint32_t,std::vector<uint32_t>> *_placePostTransitionMap;
                std::unordered_map<uint32_t,std::vector<uint32_t>> *_placePreTransitionMap;
                std::unordered_map<uint32_t,bool> _inQueue;
                std::vector<Place> *_places;
                std::unordered_map<uint32_t, EquivalenceVec> _partition;
                const PetriEngine::Colored::IntervalGenerator intervalGenerator = IntervalGenerator();
                std::vector<uint32_t> _placeQueue;

                bool splitPartition(EquivalenceVec equivalenceVec, uint32_t placeId);

                void handleTransition(uint32_t transitionId, uint32_t postPlaceId);
                void handleTransition(const Transition &transitionId, const uint32_t postPlaceId, const Arc *postArc);

                void handleLeafTransitions();
                
                void addToQueue(uint32_t placeId);

                bool checkTupleDiagonal(uint32_t placeId);
                bool checkDiagonal(uint32_t placeId);

                void handleInArcs(const Transition &transition, std::set<const Colored::Variable*> &diagonalVars, const PositionVariableMap &varPositionMap, std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId);

                void applyNewIntervals(const Arc &inArc, std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps);

                void checkVarOnArc(const VariableModifierMap &varModifierMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId, bool inputArc);

                void checkVarOnInputArcs(std::unordered_map<uint32_t,PositionVariableMap> &placeVariableMap, const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                void markSharedVars(const PositionVariableMap &preVarPositionMap, const PositionVariableMap &varPositionMap, uint32_t postPlaceId, uint32_t prePlaceId);
                void checkVarInGuard(const PositionVariableMap &preVarPositionMap, const std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                std::vector<VariableIntervalMap> prepareVariables(
                            VariableModifierMap varModifierMap, 
                            const EquivalenceClass& eqClass , const Arc *postArc, uint32_t placeId);

                bool findOverlap(EquivalenceVec equivalenceVec1, EquivalenceVec equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection);

                uint32_t eqClassIdCounter = 0;

            public:
                PartitionBuilder(std::vector<Transition> *transitions, 
                                std::vector<Place> *places, 
                                std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, 
                                std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap);

                PartitionBuilder(std::vector<Transition> *transitions, 
                                std::vector<Place> *places, 
                                std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, 
                                std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap,
                                std::vector<Colored::ColorFixpoint> *placeColorFixpoints);
                
                ~PartitionBuilder() {}

                //void initPartition();
                bool partitionNet(int32_t timeout);
                void refinePartition();
                void printPartion();
                void assignColorMap(std::unordered_map<uint32_t, EquivalenceVec> &partition);

                std::unordered_map<uint32_t, EquivalenceVec> getPartition(){
                    return _partition;
                }
        };
    }
}
