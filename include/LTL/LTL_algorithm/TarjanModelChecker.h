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

namespace LTL {

    class TarjanModelChecker : public ModelChecker {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &cond)
                : ModelChecker(net, cond), successorGenerator(net, cond), factory(net, successorGenerator.initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1), store(net, 0, (int) net.numberOfPlaces() + 1) {
            chash.fill(std::numeric_limits<idx_t>::max());
        }

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        static constexpr auto MISSING = std::numeric_limits<idx_t>::max();

        LTL::ProductSuccessorGenerator successorGenerator;
        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        PetriEngine::Structures::StateSet store;

        static constexpr auto HashSz = 4096;
        std::array<idx_t, HashSz> chash;
        inline idx_t &hash_search(idx_t stateid) {
            return chash[stateid % HashSz];
        }

        struct CStack {
            size_t lowlink;
            size_t stateid;
            size_t next = std::numeric_limits<idx_t>::max();
        };

        struct DStack {
            size_t pos;
            uint32_t nexttrans = -1;
            std::optional<std::vector<size_t>> neighbors;
        };


        std::vector<CStack> cstack;
        std::stack<DStack> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state);

        void pop();

        void update(idx_t to);

        bool nexttrans(State &state, DStack &delem);
    };
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
