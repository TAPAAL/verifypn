#include "PetriEngine/Simplification/LinearPrograms.h"

#include <vector>

namespace PetriEngine {
    namespace Simplification {

        // ***********************************
        // AbstractProgramCollection functions
        // ***********************************

        bool AbstractProgramCollection::satisfiable(const PQL::SimplificationContext& context, uint32_t solvetime)
        {
            reset();
            if (context.timeout() || has_empty || solvetime == 0) return true;
            if (_result != UNKNOWN)
            {
                if (_result == IMPOSSIBLE)
                {
                    return _result == POSSIBLE;
                }
            }
            satisfiableImpl(context, solvetime);
            assert(_result != UNKNOWN);
            return _result == POSSIBLE;
        }

        uint32_t AbstractProgramCollection::explorePotency(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            return explorePotencyImpl(context, potencies, maxConfigurationsSolved);
        }

        // *************************
        // UnionCollection functions
        // *************************

        UnionCollection::UnionCollection(std::vector<AbstractProgramCollection_ptr>&& programs)
        : AbstractProgramCollection(), lps(std::move(programs))
        {
            for (auto& p : lps)
                _size += p->size();
        }

        UnionCollection::UnionCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B)
        : AbstractProgramCollection(), lps({A,B})
        {
            has_empty = false;
            for (auto& lp : lps)
            {
                has_empty = has_empty || lp->empty();
                if (lp->known_sat() || has_empty) _result = POSSIBLE;
                if (_result == POSSIBLE) break;
            }
            for (auto& p : lps)
                _size += p->size();
        }

        void UnionCollection::clear()
        {
            lps.clear();
            current = 0;
        }

        void UnionCollection::reset()
        {
            lps[0]->reset();
            current = 0;
        }

        bool UnionCollection::merge(bool& has_empty, LinearProgram& program, bool dry_run)
        {
            if (current >= lps.size())
            {
                current = 0;
            }

            if (!lps[current]->merge(has_empty, program, dry_run))
            {
                ++current;
                if (current < lps.size())
                    lps[current]->reset();
            }

            return current < lps.size();
        }

        void UnionCollection::satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
        {
            for (int i = lps.size() - 1; i >= 0; --i)
            {
                if (lps[i]->satisfiable(context, solvetime) || context.timeout())
                {
                    _result = POSSIBLE;
                    return;
                }
                else
                {
                    lps.erase(lps.begin() + i);
                }
            }
            if (_result != POSSIBLE)
                _result = IMPOSSIBLE;
        }

        uint32_t UnionCollection::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            for (int i = lps.size() - 1; i >= 0 && maxConfigurationsSolved > 0; --i)
            {
                if (context.potencyTimeout())
                    return 0;

                maxConfigurationsSolved = lps[i]->explorePotency(context, potencies, maxConfigurationsSolved);
                lps.erase(lps.begin() + i); // Continue to the next configuration after erasing the one we just solved
            }

            return maxConfigurationsSolved;
        }

        // constexpr uint16_t MAX_CONFIG = 10;

        // *************************
        // MergeCollection functions
        // *************************

        MergeCollection::MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B)
        : AbstractProgramCollection(), left(A), right(B)
        {
            assert(A);
            assert(B);
            has_empty = left->empty() && right->empty();
            _size = left->size() * right->size();
        }

        void MergeCollection::clear()
        {
            left = nullptr;
            right = nullptr;
        }

        void MergeCollection::reset()
        {
            if (right)
                right->reset();

            merge_right = true;
            more_right  = true;
            rempty = false;

            tmp_prog = LinearProgram();
            curr = 0;
        }

        bool MergeCollection::merge(bool& has_empty, LinearProgram& program, bool dry_run)
        {
            if (program.knownImpossible()) {
                return false;
            }

            bool lempty = false;
            bool more_left;
            while (true)
            {
                lempty = false;
                LinearProgram prog = program;
                if (merge_right)
                {
                    assert(more_right);
                    rempty = false;
                    tmp_prog = LinearProgram();
                    more_right = right->merge(rempty, tmp_prog, false);
                    left->reset();
                    merge_right = false;
                }

                assert(curr < _size);

                more_left = left->merge(lempty, prog/*, dry_run || curr < nsat*/);
                if (!more_left) merge_right = true;
                if (curr >= nsat || !(more_left || more_right))
                {
                    if ((!dry_run && prog.knownImpossible()) && (more_left || more_right)) {
                        continue;
                    }

                    if (!dry_run) {
                        program.swap(prog);
                    }

                    ++curr;
                    break;
                }
                
                ++curr;
            }
            if (!dry_run)
                program.make_union(tmp_prog);
            has_empty = lempty && rempty;
            return more_left || more_right;
        }

        void MergeCollection::satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
        {
            // this is where the magic needs to happen
            bool hasmore = false;
            do {
                if (context.timeout())
                {
                    _result = POSSIBLE;
                    break;
                }

                LinearProgram prog;
                bool has_empty = false;
                hasmore = merge(has_empty, prog);
                if (has_empty)
                {
                    _result = POSSIBLE;
                    return;
                }
                else
                {
                    if (context.timeout() ||
                        !prog.isImpossible(context, solvetime))
                    {
                        _result = POSSIBLE;
                        break;
                    }
                }
                ++nsat;
            } while (hasmore);
            if (_result != POSSIBLE)
                _result = IMPOSSIBLE;
        }

        uint32_t MergeCollection::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            bool hasmore = false;
            do {
                if (context.potencyTimeout() || maxConfigurationsSolved == 0)
                    return 0;

                LinearProgram prog;
                bool has_empty = false;
                hasmore = merge(has_empty, prog);
                if (has_empty)
                    return maxConfigurationsSolved;
                else
                {
                    --maxConfigurationsSolved;
                    prog.solvePotency(context, potencies);
                }

                ++nsat;
            } while (hasmore);

            return maxConfigurationsSolved;
        }

        // ***********************
        // SingleProgram functions
        // ***********************

        SingleProgram::SingleProgram() : AbstractProgramCollection()
        {
            has_empty = true;
        }

        SingleProgram::SingleProgram(LPCache* factory, const Member& lh, int64_t constant, op_t op)
        : AbstractProgramCollection(),
          program(factory->createAndCache(lh.variables()), constant, op, factory)
        {
            has_empty = program.size() == 0;
            assert(!has_empty);
        }

        bool SingleProgram::merge(bool& has_empty, LinearProgram& program, bool dry_run)
        {
            if (dry_run)
                return false;
            program.make_union(this->program);
            has_empty = this->program.equations().size() == 0;
            assert(has_empty == this->has_empty);
            return false;
        }

        void SingleProgram::satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime)
        {
            // this is where the magic needs to happen
            if (!program.isImpossible(context, solvetime))
            {
                _result = POSSIBLE;
            }
            else
            {
                _result = IMPOSSIBLE;
            }
        }

        uint32_t SingleProgram::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            if (context.potencyTimeout() || maxConfigurationsSolved == 0)
                return 0;

            program.solvePotency(context, potencies);
            return maxConfigurationsSolved - 1;
        }
    }
}
