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
        struct place_t {
            uint32_t producers, consumers;
        };

        struct trans_t {
            uint32_t index;
            int8_t direction;

            trans_t() = default;

            trans_t(uint32_t id, int8_t dir) : index(id), direction(dir) {};

            bool operator<(const trans_t &t) const {
                return index < t.index;
            }
        };

    private:
        void setupProducersAndConsumers();

        void setupExtBounds();

        [[nodiscard]] std::pair<const trans_t*, const trans_t*> producers(uint32_t p) const;
        [[nodiscard]] std::pair<const trans_t*, const trans_t*> consumers(uint32_t p) const;

        void findDeadPlacesAndTransitions(const PetriEngine::Marking *marking);

        void extrapolateDynamicReachRelevance(PetriEngine::Marking *marking, PetriEngine::Condition *query);

        void findDynamicVisiblePlaces(PetriEngine::Condition *query);

        void extrapolateStaticReachRelevance(PetriEngine::Marking *marking, PetriEngine::Condition *query);

        const std::vector<bool> &findStaticVisiblePlaces(PetriEngine::Condition *query);

    private:
        // === Settings
        bool _enabled = true;
        bool _doDynamic = true;
        bool _env_TOKEN_ELIM_DEBUG = std::getenv("TOKEN_ELIM_DEBUG") != nullptr;

        // === Net
        PetriNet const *_net;
        std::vector<uint32_t> _extBounds;
        std::unique_ptr<place_t[]> _prodcons;
        std::unique_ptr<trans_t[]> _parcs;
        std::vector<std::vector<uint32_t>> _inhibpost;

        // === Cache and working flags
        std::unordered_map<const Condition *, const std::vector<bool>> _cache;
        std::vector<uint8_t> _pflags;
        std::vector<bool> _fireable;

        // === Statistics
        size_t _tokensExtrapolated = 0;
    };
}

#endif //VERIFYPN_EXTRAPOLATOR_H
