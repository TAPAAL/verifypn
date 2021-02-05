/*
 * File:   ReachabilityStubbornSet.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 03/02/2021
 */

#ifndef VERIFYPN_REACHABILITYSTUBBORNSET_H
#define VERIFYPN_REACHABILITYSTUBBORNSET_H


#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {
    class ReachabilityStubbornSet : public StubbornSet {
    public:
        ReachabilityStubbornSet(const PetriNet &net, const vector<PQL::Condition_ptr> &queries)
                : StubbornSet(net, queries) {}

        ReachabilityStubbornSet(const PetriNet &net)
                : StubbornSet(net) {}

        void prepare(const Structures::State *state) override;


    private:
    };
}

#endif //VERIFYPN_REACHABILITYSTUBBORNSET_H
