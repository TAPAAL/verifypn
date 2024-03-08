#ifndef VERIFYPN_EXTRAPOLATOR_H
#define VERIFYPN_EXTRAPOLATOR_H


#include <utility>

#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PQL/PQL.h"
#include "CTL/PetriNets/PetriConfig.h"

namespace PetriEngine {
    using Condition = PQL::Condition;
    using Condition_ptr = PQL::Condition_ptr;
    using Marking = Structures::State;

    const static uint8_t IN_Q_INC = 1 << 0;
    const static uint8_t IN_Q_DEC = 1 << 1;
    const static uint8_t VIS_INC = 1 << 2;
    const static uint8_t VIS_DEC = 1 << 3;
    const static uint8_t MUST_KEEP = 1 << 4;
    const static uint8_t CAN_INC = 1 << 5;
    const static uint8_t CAN_DEC = 1 << 6;

    class Extrapolator {
    public:
        virtual ~Extrapolator() = default;

        void init(const PetriNet *net, const Condition *query);

        void extrapolate(Marking *marking, Condition *query);

        [[nodiscard]] virtual size_t tokensExtrapolated() const {
            return _tokensExtrapolated;
        }

        Extrapolator * disable() {
            _enabled = false;
            return this;
        }

        Extrapolator * setDynamic(bool dynamic) {
            _doDynamic = dynamic;
            return this;
        }

    private:
        void setupProducersAndConsumers();

        void setupExtBounds();

        [[nodiscard]] int effect(uint32_t t, uint32_t p) const;

        void findDeadPlacesAndTransitions(const PetriEngine::Marking *marking);

        void extrapolateDynamicReachRelevance(PetriEngine::Marking *marking, PetriEngine::Condition *query);

        void findDynamicVisiblePlaces(PetriEngine::Condition *query);

        void extrapolateStaticReachRelevance(PetriEngine::Marking *marking, PetriEngine::Condition *query);

        const std::vector<bool> &findStaticVisiblePlaces(PetriEngine::Condition *query);

    private:
        // === Settings
        bool _enabled = true;
        bool _doDynamic = false;
        bool _env_DYN_EXTRAP_DEBUG = std::getenv("DYN_EXTRAP_DEBUG") != nullptr;

        // === Net
        PetriNet const *_net;
        std::vector<uint32_t> _extBounds;
        std::vector<std::vector<uint32_t>> _producers;
        std::vector<std::vector<uint32_t>> _consumers;

        // === Cache and working flags
        std::unordered_map<const Condition *, const std::vector<bool>> _cache;
        std::vector<uint8_t> _pflags;
        std::vector<bool> _fireable;

        // === Statistics
        size_t _tokensExtrapolated = 0;
    };
}

#endif //VERIFYPN_EXTRAPOLATOR_H
