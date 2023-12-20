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

    const static uint8_t IN_Q = 0b000001;
    const static uint8_t VIS_INC = 0b000010;
    const static uint8_t VIS_DEC = 0b000100;
    const static uint8_t MUST_KEEP = 0b001000;
    const static uint8_t CAN_INC = 0b010000;
    const static uint8_t CAN_DEC = 0b100000;

    struct ExtrapolationContext {
        PetriNet const *net;
        std::vector<uint32_t> upperBounds;
        std::vector<std::vector<uint32_t>> producers;
        std::vector<std::vector<uint32_t>> consumers;

        explicit ExtrapolationContext(const PetriNet *net);

        [[nodiscard]] int effect(uint32_t t, uint32_t p) const;

    private:
        void setupProducersAndConsumers();
        void setupUpperBounds();
    };

    using ExtrapolationContext_cptr = std::shared_ptr<const ExtrapolationContext>;

    class Extrapolator {
    public:
        virtual ~Extrapolator() = default;

        virtual void init(const PetriNet *net, const Condition *query) {
            if (!_ctx) {
                _ctx = std::make_shared<ExtrapolationContext>(net);
            }
        };

        virtual void initWithCtx(const ExtrapolationContext_cptr &ctx, const Condition *query) {
            _ctx = ctx;
            init(_ctx->net, query);
        }

        virtual void extrapolate(Marking *marking, Condition *query) = 0;

        [[nodiscard]] virtual size_t tokensExtrapolated() const {
            return _tokensExtrapolated;
        }

    protected:
        ExtrapolationContext_cptr _ctx;
        size_t _tokensExtrapolated = 0;
    };

    class NoExtrapolator : public Extrapolator {
    public:
        void extrapolate(Marking *marking, Condition *query) override {
            // no-op
        };
    };

    /**
     * The SimpleReachExtrapolator removes tokens in places that are not visible for the query,
     * neither directly or indirectly. The SimpleReachExtrapolator preserves reachability/safety properties.
     */
    class SimpleReachExtrapolator : public Extrapolator {
    public:
        void extrapolate(Marking *marking, Condition *query) override;

    protected:
        const std::vector<bool> &findVisiblePlaces(Condition *query);

        std::unordered_map<const Condition *, const std::vector<bool>> _cache;
    };

    /**
     * The DynamicReachExtrapolator removes tokens in places that are *effectively* not visible for the query,
     * neither directly or indirectly, by considering the current marking and
     * which places can never gain or lose tokens.
     * The DynamicReachExtrapolator preserves reachability/safety properties.
     */
    class DynamicReachExtrapolator : public Extrapolator {
    public:
        void extrapolate(Marking *marking, Condition *query) override;

    protected:

        std::vector<uint8_t> _pflags;
        std::vector<bool> _fireable;

        void findDeadPlacesAndTransitions(const Marking *marking);
        void findVisiblePlaces(Condition *query);
    };

    class AdaptiveExtrapolator : public Extrapolator {
    public:
        void init(const PetriNet *net, const Condition *query) override;

        void extrapolate(Marking *marking, Condition *query) override;

        size_t tokensExtrapolated() const override;

    protected:
        //CTLExtrapolator _ctl;
        DynamicReachExtrapolator _simple;
    };
}

#endif //VERIFYPN_EXTRAPOLATOR_H
