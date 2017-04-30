#include "LinearProgram.h"
#include <assert.h>
#include "../../lpsolve/lp_lib.h"

namespace PetriEngine {
    namespace Simplification {
        LinearProgram::LinearProgram() {
        }

        LinearProgram::~LinearProgram() {
        }
        
        LinearProgram::LinearProgram(Equation&& eq){
            equations.emplace_back(std::make_shared<Equation>(std::move(eq)));
        }

        bool LinearProgram::isImpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout){
            if(equations.size() == 0){
                return false;
            }
            
            if(_result != result_t::UKNOWN) 
            {
                return _result == result_t::IMPOSSIBLE;
            }

            const uint32_t nCol = net->numberOfTransitions();
            lprec* lp;
            int nRow = net->numberOfPlaces() + equations.size();
            
            lp = make_lp(nRow, nCol);
            assert(lp);
            if (!lp) return false;
            set_verbose(lp, IMPORTANT);

            set_add_rowmode(lp, TRUE);
            
            std::vector<REAL> row = std::vector<REAL>(nCol + 1);
            REAL constant = 0;
            int comparator = GE;
            
            int rowno = 1;
            
            // restrict all places to contain 0+ tokens
            for (size_t p = 0; p < net->numberOfPlaces(); p++) {
                memset(row.data(), 0, sizeof (REAL) * (nCol + 1));
                for (size_t t = 0; t < nCol; t++) {
                    row[1 + t] = net->outArc(t, p) - net->inArc(p, t);
                }
                set_row(lp, rowno, row.data());
                set_constr_type(lp, rowno, GE);
                set_rh(lp, rowno++, (0 - (int)m0[p]));
            }

            for(auto& eq : equations){
                switch(eq->op)
                {
                    case Equation::OP_LT:
                        constant = (REAL) (eq->constant - 1);
                        comparator = LE;
                        break;
                    case Equation::OP_GT:
                        constant = (REAL) (eq->constant + 1);
                        comparator = GE;
                        break;
                    case Equation::OP_LE:
                        constant = (REAL) eq->constant;
                        comparator = LE;
                        break;
                    case Equation::OP_GE:
                        constant = (REAL) eq->constant;
                        comparator = GE;
                        break;
                    case Equation::OP_EQ:
                        constant = (REAL) eq->constant;
                        comparator = EQ;
                        break;
                    default:
                        // We ignore this operator for now by not adding any equation.
                        // This is untested, however.
                        assert(false);
                        continue;
                }
                memset(row.data(), 0, sizeof (REAL) * (nCol + 1));
                for (size_t t = 1; t < nCol+1; t++) {
                    assert((t - 1) < eq->row.size());
                    row[t] = (REAL)eq->row[t - 1]; // first index is for lp-solve.
                }
                
                set_row(lp, rowno, row.data());
                set_constr_type(lp, rowno, comparator);
                set_rh(lp, rowno++, constant);
            }
            set_add_rowmode(lp, FALSE);
            
            // Create objective
            memset(row.data(), 0, sizeof (REAL) * net->numberOfTransitions() + 1);
            for (size_t t = 0; t < net->numberOfTransitions(); t++)
                row[1 + t] = 1; // The sum the components in the firing vector

            // Set objective
            set_obj_fn(lp, row.data());

            // Minimize the objective
            set_minim(lp);

            for (size_t i = 0; i < nCol; i++){
                set_int(lp, 1 + i, TRUE);
            }
            
            set_timeout(lp, timeout);
            set_break_at_first(lp, TRUE);
            set_presolve(lp, PRESOLVE_ROWS | PRESOLVE_COLS | PRESOLVE_LINDEP, get_presolveloops(lp));
//            write_LP(lp, stdout);
            int result = solve(lp);
            delete_lp(lp);

            if (result == TIMEOUT) std::cout << "note: lpsolve timeout" << std::endl;
            // Return true, if it was infeasible
            if(result == INFEASIBLE)
            {
                _result = result_t::IMPOSSIBLE;
            }
            else
            {
                _result = result_t::POSSIBLE;
            }
            return _result == result_t::IMPOSSIBLE;
        }
    }
}

