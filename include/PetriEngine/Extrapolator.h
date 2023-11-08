#ifndef VERIFYPN_EXTRAPOLATOR_H
#define VERIFYPN_EXTRAPOLATOR_H


#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PQL/PQL.h"
#include "CTL/PetriNets/PetriConfig.h"

namespace PetriEngine {
    using Condition = PQL::Condition;
    using Condition_ptr = PQL::Condition_ptr;
    using Marking = Structures::State;

    class Extrapolator {
    public:
        virtual ~Extrapolator() = default;

        virtual void init(const PetriEngine::PetriNet *net, const Condition *query) = 0;

        virtual void extrapolate(Marking *marking, Condition *query) = 0;

        static std::vector<uint32_t> findUpperBounds(const PetriEngine::PetriNet *net);

        static std::pair<std::vector<std::vector<uint32_t>>, std::vector<std::vector<uint32_t>>>
        findProducersAndConsumers(const PetriEngine::PetriNet *net);

        size_t tokensExtrapolated() const {
            return _tokensExtrapolated;
        }

    protected:
        size_t _tokensExtrapolated = 0;
    };

    class NoExtrapolator : public Extrapolator {
    public:
        void init(const PetriEngine::PetriNet *net, const Condition *query) override {
            // no-op
        };

        void extrapolate(Marking *marking, Condition *query) override {
            // no-op
        };
    };

    /**
     * The SimpleExtrapolator removes tokens in places that are not visible for the query,
     * neither directly or indirectly.
     */
    class SimpleExtrapolator : public Extrapolator {
    public:
        void init(const PetriEngine::PetriNet *net, const Condition *query) override;

        void extrapolate(Marking *marking, Condition *query) override;

    protected:
        const std::vector<bool> & findVisiblePlaces(Condition *query);

        bool _initialized = false;
        PetriEngine::PetriNet const *_net;
        std::vector<uint32_t> _upperBounds;
        std::vector<std::vector<uint32_t>> _producers;
        std::vector<std::vector<uint32_t>> _consumers;
        std::unordered_map<const Condition *, const std::vector<bool>> _cache;
    };

    /**
     * The SmartExtrapolator removes tokens in places that are *effectively* not visible for the query,
     * neither directly or indirectly, by considering the current marking and
     * which places can never gain or lose tokens.
     */
    class SmartExtrapolator : public Extrapolator {
    public:
        void init(const PetriEngine::PetriNet *net, const Condition *query) override;

        void extrapolate(Marking *marking, Condition *query) override;
    };
}

#endif //VERIFYPN_EXTRAPOLATOR_H
