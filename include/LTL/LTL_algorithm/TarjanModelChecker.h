/*
 * File:   TarjanModelChecker.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#ifndef VERIFYPN_TARJANMODELCHECKER_H
#define VERIFYPN_TARJANMODELCHECKER_H


#include "LTL/LTL_algorithm/ModelChecker.h"
#include "LTL/Structures/ProductStateFactory.h"
#include "PetriEngine/Structures/StateSet.h"

#include <stack>
#include <unordered_set>

namespace LTL {

    template <bool SaveTrace>
    class TarjanModelChecker : public ModelChecker {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &cond, const bool shortcircuitweak)
                : ModelChecker(net, cond, shortcircuitweak), factory(net, successorGenerator->initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1) {
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;
        using idx_t = uint32_t;
        // 64 MB hash table
        static constexpr idx_t HashSz = 16777216;

        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        std::unordered_set<idx_t> store;

        std::array<idx_t, HashSz> chash;
        static_assert(sizeof(chash) == (1U << 26U));

        static inline idx_t hash(idx_t id) {
            return id % HashSz;
        }

        struct PlainCEntry {
            idx_t lowlink;
            idx_t stateid;
            idx_t next = std::numeric_limits<idx_t>::max();

            PlainCEntry(idx_t lowlink, idx_t stateid, idx_t next) : lowlink(lowlink), stateid(stateid), next(next) {}
        };

        struct TracableCEntry : PlainCEntry {
            idx_t lowsource = std::numeric_limits<idx_t>::max();
            uint32_t sourcetrans;

            TracableCEntry(idx_t lowlink, idx_t stateid, idx_t next) : PlainCEntry(lowlink, stateid, next) {}
        };

        using CEntry = std::conditional_t<SaveTrace,
            TracableCEntry,
            PlainCEntry>;

        struct DEntry {
            idx_t pos;
            successor_info sucinfo;
        };


        std::vector<CEntry> cstack;
        std::stack<DEntry> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, State& parent, DEntry &delem);

        void popCStack();
    };
extern template class TarjanModelChecker<true>;
extern template class TarjanModelChecker<false>;
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
