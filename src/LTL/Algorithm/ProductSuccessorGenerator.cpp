//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/ProductSuccessorGenerator.h"

#undef PRINTF_DEBUG

namespace LTL {
#ifdef PRINTF_DEBUG
    inline void _dump_state(const LTL::Structures::ProductState &state, int nplaces=-1) {
        if (nplaces == -1) nplaces = state.buchi_state_idx;
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (int i = 1; i <= nplaces; ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }
#endif

    void ProductSuccessorGenerator::prepare(const LTL::Structures::ProductState *state) {
#ifdef PRINTF_DEBUG
        _dump_state(*state);
#endif
        SuccessorGenerator::prepare(state);
        buchi.prepare(state->getBuchiState());
        buchi_parent = state->getBuchiState();
        fresh_marking = true;
    }

    void ProductSuccessorGenerator::prepare(const LTL::Structures::ProductState *state, const successor_info &sucinfo) {
        SuccessorGenerator::prepare(state);
        buchi.prepare(state->getBuchiState());
        buchi_parent = state->getBuchiState();
        fresh_marking = sucinfo.pcounter == 0 && sucinfo.tcounter == std::numeric_limits<uint32_t>::max();
        _suc_pcounter = sucinfo.pcounter;
        _suc_tcounter = sucinfo.tcounter;
        if (!fresh_marking) {
            assert(sucinfo.buchi_state != std::numeric_limits<size_t>::max());
            // spool Büchi successors until last state found.
            // TODO is there perhaps a good way to avoid this, perhaps using raw edge vector?
            // Caveat: it seems like there usually are not that many successors, so this is probably cheap regardless
            size_t tmp;
            while (buchi.next(tmp, cond)) {
                if (tmp == sucinfo.buchi_state) {
                    break;
                }
            }
        }
    }

    bool ProductSuccessorGenerator::next(LTL::Structures::ProductState &state) {
        if (fresh_marking) {
            fresh_marking = false;
            if (!SuccessorGenerator::next(state)) {
                // This is a fresh marking, so if there is no more successors for the state the state is deadlocked.
                // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                std::copy(_parent->marking(), _parent->marking() + state.buchi_state_idx + 1, state.marking());
            }
        }
        if (next_buchi_succ(state)) {
            return true;
        }
        // No valid transition in Büchi automaton for current marking;
        // Try next marking(s) and see if we find a successor.
        else {
            while (SuccessorGenerator::next(state)) {
                // reset buchi successors
                buchi.prepare(buchi_parent);
                if (next_buchi_succ(state)) {
                    return true;
                }
            }
            return false;
        }
    }

    bool ProductSuccessorGenerator::isAccepting(const LTL::Structures::ProductState &state) {
        return buchi.is_accepting(state.getBuchiState());
    }

    bool ProductSuccessorGenerator::next_buchi_succ(LTL::Structures::ProductState &state) {
        size_t tmp;
        while (buchi.next(tmp, cond)) {
            if (guard_valid(state, cond)) {
#ifdef PRINTF_DEBUG
                std::cerr << "Satisfied guard: " << cond << std::endl;
#endif
                state.setBuchiState(tmp);
                return true;
            }
        }
        return false;
    }

    bool ProductSuccessorGenerator::guard_valid(const PetriEngine::Structures::State &state, bdd bdd) {
        EvaluationContext ctx{state.marking(), &_net};
        // IDs 0 and 1 are false and true atoms, respectively
        while (bdd.id() > 1/*!(bdd == bddtrue || bdd == bddfalse)*/) {
            // find variable to test, and test it
            size_t var = bdd_var(bdd);
            Condition::Result res = buchi.getExpression(var)->evaluate(ctx);
            switch (res) {
                case Condition::RUNKNOWN:
                    std::cerr << "Unexpected unknown answer from evaluating query!\n";
                    assert(false);
                    exit(1);
                    break;
                case Condition::RFALSE:
                    bdd = bdd_low(bdd);
                    break;
                case Condition::RTRUE:
                    bdd = bdd_high(bdd);
                    break;
            }
        }
        return bdd == bddtrue;
    }

    bool ProductSuccessorGenerator::next(Structures::ProductState &state, successor_info &sucinfo) {
        auto update_sucinfo = [&]() {
            sucinfo.pcounter = _suc_pcounter;
            sucinfo.tcounter = _suc_tcounter;
            sucinfo.buchi_state = state.getBuchiState();
        };

        if (fresh_marking) {
            fresh_marking = false;
            if (!SuccessorGenerator::next(state)) {
                // This is a fresh marking, so if there are no more successors for the state the state is deadlocked.
                // The semantics for deadlock is to just loop the marking so return true without changing the value of state.
                std::copy(_parent->marking(), _parent->marking() + state.buchi_state_idx + 1, state.marking());
            }
        }
        if (next_buchi_succ(state)) {
            update_sucinfo();
            return true;
        }
        // No valid transition in Büchi automaton for current marking;
        // Try next marking(s) and see if we find a successor.
        else {
            while (SuccessorGenerator::next(state)) {
                // reset buchi successors
                buchi.prepare(buchi_parent);
                if (next_buchi_succ(state)) {
                    update_sucinfo();
                    return true;
                }
            }
            return false;
        }
    }

}
