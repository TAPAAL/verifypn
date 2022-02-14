#include "ColoredNetStructures.h"
#include "EquivalenceVec.h"
#include "IntervalGenerator.h"

namespace PetriEngine {
    namespace Colored {
        class PartitionBuilder {


            public:
                PartitionBuilder(const std::vector<Transition> &transitions,
                                const std::vector<Place> &places);

                PartitionBuilder(const std::vector<Transition> &transitions,
                                const std::vector<Place> &places,
                                const std::vector<Colored::ColorFixpoint> *placeColorFixpoints);

                ~PartitionBuilder() {}

                //void initPartition();
                bool partitionNet(int32_t timeout);
                void printPartion() const;
                void assignColorMap(std::vector<EquivalenceVec> &partition) const;

                const std::vector<EquivalenceVec>& getPartition() const{
                    return _partition;
                }

            private:
                const std::vector<Transition> &_transitions;
                const std::vector<Place> &_places;
                std::vector<bool> _inQueue;
                std::vector<EquivalenceVec> _partition;
                const PetriEngine::Colored::IntervalGenerator _interval_generator = IntervalGenerator();
                std::vector<uint32_t> _placeQueue;

                bool splitPartition(EquivalenceVec equivalenceVec, uint32_t placeId);

                void handleTransition(uint32_t transitionId, uint32_t postPlaceId);
                void handleTransition(const Transition &transitionId, const uint32_t postPlaceId, const Arc *postArc);

                void handleLeafTransitions();

                void addToQueue(uint32_t placeId);

                bool checkTupleDiagonal(uint32_t placeId);
                bool checkDiagonal(uint32_t placeId);

                void handleInArcs(const Transition &transition, std::set<const Colored::Variable*> &diagonalVars, const PositionVariableMap &varPositionMap, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId);

                void applyNewIntervals(const Arc &inArc, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps);

                void checkVarOnArc(const VariableModifierMap &varModifierMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId, bool inputArc);

                void checkVarOnInputArcs(const std::unordered_map<uint32_t,PositionVariableMap> &placeVariableMap, const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                void markSharedVars(const PositionVariableMap &preVarPositionMap, const PositionVariableMap &varPositionMap, uint32_t postPlaceId, uint32_t prePlaceId);
                void checkVarInGuard(const PositionVariableMap &preVarPositionMap, const std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                std::vector<VariableIntervalMap> prepareVariables(
                            const VariableModifierMap &varModifierMap,
                            const EquivalenceClass& eqClass , const Arc *postArc, uint32_t placeId);

                bool findOverlap(const EquivalenceVec &equivalenceVec1,const EquivalenceVec &equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection);

                uint32_t _eq_id_counter = 0;

        };
    }
}
