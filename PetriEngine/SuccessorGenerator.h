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
namespace PetriEngine {
class SuccessorGenerator {
public:
    SuccessorGenerator(const PetriNet& net, const Structures::State& state);
    virtual ~SuccessorGenerator();
    bool next(Structures::State& write);
    uint32_t fired()
    {
        return _suc_tcounter -1;
    }
private:
    const PetriNet& _net;
    const Structures::State& _parent;
    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;
};
}

#endif /* SUCCESSORGENERATOR_H */

