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

    class SuccessorGenerator {
public:
    SuccessorGenerator(const PetriNet& net);
    SuccessorGenerator(const PetriNet& net, const std::shared_ptr<StubbornSet>&);
    SuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries);
    SuccessorGenerator(const PetriNet& net, const std::shared_ptr<PQL::Condition> &query);
    virtual ~SuccessorGenerator();
    virtual bool prepare(const Structures::State& state) { return prepare(&state); }
    virtual bool prepare(const Structures::State* state);
    virtual bool next(Structures::State& write)
    {
        return _next(write, [](size_t){ return true; });
    }

    uint32_t fired() const
    {
        return _suc_tcounter == std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : _suc_tcounter - 1;
    }

    const MarkVal* getParent() const {
        return _parent->marking();
    }

    const PetriNet& net() const { return _net; }

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

protected:
    const PetriNet& _net;

    bool next(Structures::State &write, uint32_t &tindex);

    void _fire(Structures::State &write, uint32_t tid);

    template<typename T>
    bool _next(Structures::State& write, T&& predicate) {
        for (; _suc_pcounter < _net._nplaces; ++_suc_pcounter) {
            // orphans are currently under "place 0" as a special case
            if (_suc_pcounter == 0 || (*_parent).marking()[_suc_pcounter] > 0) {
                if (_suc_tcounter == std::numeric_limits<uint32_t>::max()) {
                    _suc_tcounter = _net._placeToPtrs[_suc_pcounter];
                }
                uint32_t last = _net._placeToPtrs[_suc_pcounter + 1];
                for (; _suc_tcounter != last; ++_suc_tcounter) {

                    if (!checkPreset(_suc_tcounter)) continue;
                    if (!predicate(_suc_tcounter)) continue;
                    _fire(write, _suc_tcounter); // <-- also updated _suc_tcounter
                    return true;
                }
                _suc_tcounter = std::numeric_limits<uint32_t>::max();
            }
            _suc_tcounter = std::numeric_limits<uint32_t>::max();
        }
        return false;
    }

    const Structures::State* _parent;

    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;

private:

    friend class ReducingSuccessorGenerator;

};
}

#endif /* SUCCESSORGENERATOR_H */

