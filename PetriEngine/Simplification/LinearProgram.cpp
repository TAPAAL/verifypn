#include "LinearProgram.h"
#include <assert.h>
#include "../../lpsolve/lp_lib.h"

namespace PetriEngine {
    namespace Simplification {
        LinearProgram::LinearProgram() {
        }

        LinearProgram::~LinearProgram() {
        }
        
        LinearProgram::LinearProgram(Equation eq){
            addEquation(eq);
        }
        
        void LinearProgram::addEquation(Equation eq){
            if(eq.op == "<"){
                eq.constant += 1; 
                eq.op = "<="; 
            } else if(eq.op == ">"){
                eq.constant -= 1; 
                eq.op = ">="; 
            } else if(eq.op == "!="){
                Equation eq2(eq);
                eq2.op = "<";
                eq.op = ">"; 
                addEquation(eq);
                addEquation(eq2);
                return;
            }

            equations.push_back(eq);
        }

        void LinearProgram::addEquations(std::vector<Equation> eqs){
            for(Equation& eq : eqs){
                equations.push_back(eq);
            }
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
            lp = make_lp(0, nCol);
            assert(lp);
            if (!lp) return false;
            set_verbose(lp, IMPORTANT);

            set_add_rowmode(lp, TRUE);
            std::vector<REAL> row = std::vector<REAL>(nCol + 1);

            // restrict all places to contain 0+ tokens
            for (size_t p = 0; p < net->numberOfPlaces(); p++) {
                memset(row.data(), 0, sizeof (REAL) * nCol + 1);
                for (size_t t = 0; t < nCol; t++) {
                    row[1 + t] = net->outArc(t, p) - net->inArc(p, t);
                }
                add_constraint(lp, row.data(), GE, (0 - (int)m0[p]));
            }

            for(Equation& eq : equations){
                add_constraint(lp, eq.row.data(), op(eq.op), eq.constant);
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
        //    write_LP(lp, stdout);
            int result = solve(lp);
            delete_lp(lp);

            if (result == TIMEOUT) std::cout<<"note: lpsolve timeout"<<std::endl;
            // Return true, if it was infeasible
            return result == INFEASIBLE;   
        }
    }
}

