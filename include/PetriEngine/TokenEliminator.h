#ifndef VERIFYPN_TOKENELIMINATOR_H
#define VERIFYPN_TOKENELIMINATOR_H


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

    /**
     * The TokenEliminator removes tokens from a marking based on impossible, visible, and directional effects
     * while preserving reachability properties. It will do nothing when given a query of a different kind.
     * The static setting does not take the current marking into account and will often be much faster
     * (but less effective) on repeated invocations involving the same reachability queries due to caching.
     *
     * The abstraction provided through token elimination is similar to that of structural reduction rule I+M
     * as well as stubborn reductions, but can additionally be used on-the-fly in state space searches to
     * simplify states and thus reduce the size of the state space.
     *
     * The TokenEliminator is based on the work in 'Token Elimination in Model Checking of Petri Nets' (TACAS'25)
     * by Nicolaj Ø. Jensen, Kim G. Larsen, and Jiri Srba.
     */
    class TokenEliminator {
    public:
        virtual ~TokenEliminator() = default;

        void init(const PetriNet *net, const Condition *query);

        void eliminate(Marking *marking, Condition *query);

        [[nodiscard]] virtual size_t tokensEliminated() const {
            return _tokensEliminated;
        }

        TokenEliminator * disable() {
            _enabled = false;
            return this;
        }

        TokenEliminator * setDynamic(bool dynamic) {
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

        void eliminateDynamic(PetriEngine::Marking * marking, PetriEngine::Condition * query);

        void findDynamicVisiblePlaces(PetriEngine::Condition *query);

        void eliminateStatic(PetriEngine::Marking * marking, PetriEngine::Condition * query);

        const std::vector<bool> &findStaticVisiblePlaces(PetriEngine::Condition *query);

    private:
        // === Settings
        bool _enabled = std::getenv("TAPAAL_TOKEN_ELIM") != nullptr;
        bool _doDynamic = std::getenv("TAPAAL_TOKEN_ELIM_STATIC") == nullptr;
        bool _env_TOKEN_ELIM_DEBUG = std::getenv("TAPAAL_TOKEN_ELIM_DEBUG") != nullptr;

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
        size_t _tokensEliminated = 0;
    };
}

#endif //VERIFYPN_TOKENELIMINATOR_H
