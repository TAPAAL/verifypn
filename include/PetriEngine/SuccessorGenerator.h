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
#include "Stubborn/StubbornSet.h"

namespace PetriEngine {
    /**
     * type holding sufficient information to resume successor generation for a state from a given point.
     */
    struct successor_info {
        uint32_t pcounter;
        uint32_t tcounter;
        size_t buchi_state;
        size_t last_state;

        friend bool operator==(const successor_info &lhs, const successor_info &rhs) {
            return lhs.pcounter == rhs.pcounter &&
                   lhs.tcounter == rhs.tcounter &&
                   lhs.buchi_state == rhs.buchi_state &&
                   lhs.last_state == rhs.last_state;
        }

        friend bool operator!=(const successor_info &lhs, const successor_info &rhs) {
            return !(rhs == lhs);
        }

        inline bool has_pcounter() const {
            return pcounter != NoPCounter;
        }

        inline bool has_tcounter() const {
            return tcounter != NoTCounter;
        }

        inline bool has_buchistate() const {
            return buchi_state != NoBuchiState;
        }

        inline bool has_prev_state() const {
            return last_state != NoLastState;
        }

        static constexpr auto NoPCounter = 0;
        static constexpr auto NoTCounter = std::numeric_limits<uint32_t>::max();
        static constexpr auto NoBuchiState = std::numeric_limits<size_t>::max();
        static constexpr auto NoLastState = std::numeric_limits<size_t>::max();
    };

    constexpr successor_info initial_suc_info{
            successor_info::NoPCounter,
            successor_info::NoTCounter,
            successor_info::NoBuchiState,
            successor_info::NoLastState
    };


    class SuccessorGenerator {
public:
    SuccessorGenerator(const PetriNet& net);
    SuccessorGenerator(const PetriNet& net, const std::shared_ptr<StubbornSet>&);
    SuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries);
    SuccessorGenerator(const PetriNet& net, const std::shared_ptr<PQL::Condition> &query);
    virtual ~SuccessorGenerator();
    void prepare(const Structures::State* state);
    void prepare(const Structures::State* state, const successor_info &sucinfo);
    void getSuccInfo(successor_info &sucinfo) const;
    bool next(Structures::State& write);
    uint32_t fired() const
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

    size_t last_transition() const { return _suc_tcounter == std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : _suc_tcounter - 1; }

protected:
    const PetriNet& _net;

    bool next(Structures::State &write, uint32_t &tindex);

    const Structures::State* _parent;

    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;

private:

    friend class ReducingSuccessorGenerator;

};
}

#endif /* SUCCESSORGENERATOR_H */

