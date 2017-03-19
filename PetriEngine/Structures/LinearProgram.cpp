#include "LinearProgram.h"
#include <assert.h>
#define OVER_APPROX_TIMEOUT    30

LinearProgram::LinearProgram() {
}

LinearProgram::~LinearProgram() {
}

bool LinearProgram::isimpossible(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0){
//    int nCol = net.numberOfTransitions();
    int nCol = 3;
    lprec* lp;
    lp = make_lp(0, nCol);
    assert(lp);
    if (!lp) return false;
    set_verbose(lp, IMPORTANT);
    
    std::vector<int> colno;
    for(int t = 0; t<nCol; t++){
        colno.push_back(t);
    }
       
    set_add_rowmode(lp, TRUE);
    // TODO: add constraints p >= 0 for all places...
    
    std::vector<REAL> row;
    for(Equation& eq : _equations){
        // nasty work-around to make the rows 1-based instead of 0-based.
        row.clear();
        row.push_back(0);
        for(REAL& r : eq.row)
            row.push_back(r);
      
        add_constraint(lp, row.data(), eq.constr_type, eq.constant);
    }
    set_add_rowmode(lp, FALSE);
    
    for (size_t i = 0; i < nCol; i++){
        set_int(lp, 1 + i, TRUE);
    }
    
    set_timeout(lp, OVER_APPROX_TIMEOUT);
    set_presolve(lp, PRESOLVE_ROWS | PRESOLVE_COLS | PRESOLVE_LINDEP, get_presolveloops(lp));
//    write_LP(lp, stdout);
    int result = solve(lp);
    delete_lp(lp);
    
    // Return true, if it was infeasible
    return result == INFEASIBLE;   
}

