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

    struct ExtrapolationContext {
        PetriNet const *net;
        std::vector<uint32_t> upperBounds;
        std::vector<std::vector<uint32_t>> producers;
        std::vector<std::vector<uint32_t>> consumers;

        explicit ExtrapolationContext(const PetriNet *net);

    private:
        static std::pair<std::vector<std::vector<uint32_t>>, std::vector<std::vector<uint32_t>>>
        findProducersAndConsumers(const PetriNet *net);

        static std::vector<uint32_t> findUpperBounds(const PetriNet *net);
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
     * The SmartReachExtrapolator removes tokens in places that are *effectively* not visible for the query,
     * neither directly or indirectly, by considering the current marking and
     * which places can never gain or lose tokens.
     * The SimpleReachExtrapolator preserves reachability/safety properties.
     */
    class SmartReachExtrapolator : public Extrapolator {
    public:
        void extrapolate(Marking *marking, Condition *query) override;
    };

    class AdaptiveExtrapolator : public Extrapolator {
    public:
        void init(const PetriNet *net, const Condition *query) override;

        void extrapolate(Marking *marking, Condition *query) override;

        size_t tokensExtrapolated() const override;

    protected:
        CTLExtrapolator _ctl;
        SimpleReachExtrapolator _simple;
    };
}

#endif //VERIFYPN_EXTRAPOLATOR_H
