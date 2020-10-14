/*
 * File:   TarjanModelChecker.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#ifndef VERIFYPN_TARJANMODELCHECKER_H
#define VERIFYPN_TARJANMODELCHECKER_H


#include "ModelChecker.h"
#include "LTL/Structures/ProductStateFactory.h"
#include "LTL/TarjanSuccessorGenerator.h"
#include "PetriEngine/Structures/StateSet.h"

namespace LTL {
    class TarjanModelChecker : public ModelChecker {
    public:
        TarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &cond)
                : ModelChecker(net, cond), successorGenerator(net, cond), factory(net, successorGenerator.initial_buchi_state()),
                  seen(net, 0, (int) net.numberOfPlaces() + 1), store(net, 0, (int) net.numberOfPlaces() + 1) {}

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;
        using idx_t = size_t;
        static constexpr auto MISSING = std::numeric_limits<idx_t>::max();

        LTL::TarjanSuccessorGenerator successorGenerator;
        LTL::Structures::ProductStateFactory factory;

        PetriEngine::Structures::StateSet seen;
        PetriEngine::Structures::StateSet store;

        struct CStack {
            idx_t lowlink;
            idx_t stateid;
        };
        struct DStack {
            idx_t pos;
            idx_t lasttrans;
            //std::vector<idx_t> neighbors;
        };

        std::vector<CStack> cstack;
        std::stack<DStack> dstack;
        std::stack<idx_t> astack;
        bool violation = false;

        void push(State &state);

        void pop();

        void update(idx_t to);
    };
}

#endif //VERIFYPN_TARJANMODELCHECKER_H
