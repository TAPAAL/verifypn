/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SuccessorGenerator.h
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 19:50
 */

#ifndef SUCCESSORGENERATOR_H
#define SUCCESSORGENERATOR_H

#include "PetriNet.h"
#include "Structures/State.h"
#include <memory>

namespace PetriEngine {
class SuccessorGenerator {
public:
    SuccessorGenerator(const PetriNet& net);
    SuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries);
    virtual ~SuccessorGenerator();
    void prepare(const Structures::State* state);
    bool next(Structures::State& write);
    uint32_t fired()
    {
        return _suc_tcounter -1;
    }
        
    const MarkVal* parent() const {
        return _parent->marking();
    }

    void reset();
    
    /**
     * Checks if the conditions are met for fireing t, if write != NULL,
     * then also consumes tokens from write while checking
     * @param t, transition to fire
     * @param write, marking to consume from (possibly NULL)
     * @return true if t is fireable, false otherwise
     */
    bool checkPreset(uint32_t t);

    /**
     * Consumes tokens in preset of t without from marking write checking
     * @param write, a marking to consume from
     * @param t, a transition to fire
     */
    void consumePreset(Structures::State& write, uint32_t t);

    /**
     * Produces tokens in write, given by t
     * @param write, a marking to produce to
     * @param t, a transition to fire
     */
    void producePostset(Structures::State& write, uint32_t t);

private:
    const PetriNet& _net;
    const Structures::State* _parent;
    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;

    friend class ReducingSuccessorGenerator;
};
}

#endif /* SUCCESSORGENERATOR_H */

