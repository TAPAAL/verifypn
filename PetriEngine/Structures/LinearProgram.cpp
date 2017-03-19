#include "LinearProgram.h"
#include <assert.h>
#define OVER_APPROX_TIMEOUT    30

LinearProgram::LinearProgram() {
}

LinearProgram::~LinearProgram() {
}

bool LinearProgram::isimpossible(const PetriEngine::PetriNet& net, const PetriEngine::MarkVal* m0){
    int nCol = net.numberOfTransitions();
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
    std::vector<REAL> row = std::vector<REAL>(nCol + 1);
    
    // restrict all places to contain 0+ tokens
    for (size_t p = 0; p < net.numberOfPlaces(); p++) {
        memset(row.data(), 0, sizeof (REAL) * nCol + 1);
        for (size_t t = 0; t < nCol; t++) {
            int d = net.outArc(t, p) - net.inArc(p, t);
            row[1 + t] = d;
        }
        add_constraint(lp, row.data(), GE, (0 - (int)m0[p]));
    }
    
    for(Equation& eq : equations){
        // skip equations that cannot be analyzed
        if(!eq.canAnalyze){  
            continue;
        }
        // nasty work-around to make the rows 1-based instead of 0-based.
        // would be nicer if we can just insert eq.row.data() into lpsolve
        memset(row.data(), 0, sizeof (REAL) * nCol + 1);
        for(int t = 0; t < nCol; t++){
            row[t + 1] = eq.row[t];
        }
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

