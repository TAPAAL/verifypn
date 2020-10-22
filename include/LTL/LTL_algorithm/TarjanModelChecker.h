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

    class TarjanModelChecker : public ModelChecker {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &cond)
                : ModelChecker(net, cond), factory(net, successorGenerator->initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1) {
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        static constexpr idx_t HashSz = 4096;

        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        std::unordered_set<idx_t> store;

        std::array<idx_t, HashSz> chash;

        static inline idx_t hash(idx_t id) {
            return id % HashSz;
        }

        struct CEntry {
            size_t lowlink;
            size_t stateid;
            size_t next = std::numeric_limits<idx_t>::max();

            CEntry(size_t lowlink, size_t stateid, size_t next) : lowlink(lowlink), stateid(stateid), next(next) {}
        };

        struct DEntry {
            size_t pos;
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
    };
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
