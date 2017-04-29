#include "LinearProgram.h"
#include <assert.h>
#include "../../lpsolve/lp_lib.h"

namespace PetriEngine {
    namespace Simplification {
        LinearProgram::LinearProgram() {
        }

        LinearProgram::~LinearProgram() {
        }
        
        LinearProgram::LinearProgram(const Equation&& eq){
            equations.push_back(eq);
        }
        
        void LinearProgram::addEquations(std::vector<Equation>& eqs){
            equations.insert(equations.end(), eqs.begin(), eqs.end());
        }        

        int LinearProgram::op(std::string op){
            if(op == "<="){ return 1; }
            if(op == ">="){ return 2; }
            if(op == "=="){ return 3; }
            return -1;
        }

        bool LinearProgram::isImpossible(const PetriEngine::PetriNet* net, const PetriEngine::MarkVal* m0, uint32_t timeout){
            if(equations.size() == 0){
                return false;
            }

            uint32_t nCol = net->numberOfTransitions();
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
                memset(row.data(), 0, sizeof (REAL) * nCol + 1);
                for (size_t t = 0; t < nCol; t++) {
                    row[1 + t] = net->outArc(t, p) - net->inArc(p, t);
                }
                set_row(lp, rowno, row.data());
                set_constr_type(lp, rowno, GE);
                set_rh(lp, rowno++, (0 - (int)m0[p]));
            }

            for(Equation& eq : equations){
                if(eq.op == "<"){
                    constant = (REAL) (eq.constant - 1);
                    comparator = LE;
                } else if(eq.op == ">"){
                    constant = (REAL) (eq.constant + 1);
                    comparator = GE;
                } else if(eq.op == "<="){
                    constant = (REAL) eq.constant;
                    comparator = LE;
                } else if(eq.op == ">="){
                    constant = (REAL) eq.constant;
                    comparator = GE;
                } else if(eq.op == "=="){
                    constant = (REAL) eq.constant;
                    comparator = EQ;
                } else if(eq.op == "!="){
                    // We ignore this operator for now by not adding any equation.
                    // This is untested, however.
                    continue;
                }
                                
                memset(row.data(), 0, sizeof (REAL) * nCol + 1);
                for (size_t t = 1; t < nCol+1; t++) {
                    row[t] = (REAL)eq.row[t];
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

            if (result == TIMEOUT) std::cout<<"note: lpsolve timeout"<<std::endl;
            // Return true, if it was infeasible
            return result == INFEASIBLE;   
        }
    }
}

