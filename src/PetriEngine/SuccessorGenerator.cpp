/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SuccessorGenerator.cpp
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 19:50
 */

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/Structures/State.h"
#include "utils/errors.h"

#include <cassert>
namespace PetriEngine {

    SuccessorGenerator::SuccessorGenerator(const PetriNet& net)
    : _net(net), _parent(nullptr) {
        reset();
    }
    SuccessorGenerator::SuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries) : SuccessorGenerator(net){}

    SuccessorGenerator::SuccessorGenerator(const PetriNet &net, const std::shared_ptr<PQL::Condition> &query)
                                           : SuccessorGenerator(net) {

    }

    SuccessorGenerator::~SuccessorGenerator() {
    }

    bool SuccessorGenerator::prepare(const Structures::State* state, uint32_t pcounter, uint32_t tcounter) {
        _parent = state;
        _suc_pcounter = pcounter;
        _suc_tcounter = tcounter;
        return true;
    }

    void SuccessorGenerator::reset() {
        _suc_pcounter = 0;
        _suc_tcounter = std::numeric_limits<uint32_t>::max();
    }

    void SuccessorGenerator::consumePreset(Structures::State& write, uint32_t t) {

        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;
        for (; finv < linv; ++finv) {
            if(!_net._invariants[finv].inhibitor) {
                assert(write.marking()[_net._invariants[finv].place] >= _net._invariants[finv].tokens);
                write.marking()[_net._invariants[finv].place] -= _net._invariants[finv].tokens;
            }
        }
    }

    bool SuccessorGenerator::checkPreset(uint32_t t) {
        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;

        for (; finv < linv; ++finv) {
            const Invariant& inv = _net._invariants[finv];
            if ((*_parent).marking()[inv.place] < inv.tokens) {
                if (!inv.inhibitor) {
                    return false;
                }
            } else {
                if (inv.inhibitor) {
                    return false;
                }
            }
        }
        return true;
    }

    void SuccessorGenerator::producePostset(Structures::State& write, uint32_t t) {
        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.outputs;
        uint32_t linv = _net._transitions[t + 1].inputs;

        for (; finv < linv; ++finv) {
            size_t n = write.marking()[_net._invariants[finv].place];
            n += _net._invariants[finv].tokens;
            if (n >= std::numeric_limits<uint32_t>::max()) {
                throw base_error("Exceeded 2**32 limit of tokens in a single place (", n, ")");
            }
            write.marking()[_net._invariants[finv].place] = n;
        }
    }

    bool SuccessorGenerator::next(Structures::State& write, uint32_t &tindex) {
        _parent = &write;
        _suc_pcounter = 0;
        for (; _suc_pcounter < _net._nplaces; ++_suc_pcounter) {
            // orphans are currently under "place 0" as a special case
            if (_suc_pcounter == 0 || (*_parent).marking()[_suc_pcounter] > 0) {
                if (tindex == std::numeric_limits<uint32_t>::max()) {
                    tindex = _net._placeToPtrs[_suc_pcounter];
                }
                uint32_t last = _net._placeToPtrs[_suc_pcounter + 1];
                for (; tindex != last; ++tindex) {

                    if (!checkPreset(tindex)) continue;
                    _fire(write, tindex);

                    ++tindex;
                    return true;
                }
                tindex = std::numeric_limits<uint32_t>::max();
            }
            tindex = std::numeric_limits<uint32_t>::max();
        }
        return false;
    }

    void SuccessorGenerator::_fire(Structures::State &write, uint32_t tid) {
        assert(checkPreset(tid));
        _suc_tcounter = tid + 1; // make sure "fired()" call reflects this now
        memcpy(write.marking(), (*_parent).marking(), _net._nplaces * sizeof (MarkVal));
        consumePreset(write, tid);
        producePostset(write, tid);
    }

    SuccessorGenerator::SuccessorGenerator(const PetriNet &net, const std::shared_ptr<StubbornSet>&)
        : SuccessorGenerator(net){}

}

