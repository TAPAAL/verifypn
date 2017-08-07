#include "STSolver.h"
#include "../lpsolve/lp_lib.h"
#include <assert.h>

namespace PetriEngine {     
    #define VarPlace(p,i) (((_net._nplaces * (i)) + (p)) + 1)
    #define VarPostT(t,i) ((_nPlaceVariables + (_net._ntransitions * (i)) + (t)) + 1)
   
    // return values
    #define WEIGHTEDARCS             -6
    #define INHIBITORARCS            -7

    void STSolver::MakeConstraint(std::vector<STVariable> constraint, int constr_type, REAL rh){
        int count = constraint.size();
        REAL* row = new REAL[count];
        int* col = new int[count];
        for(int c = 0; c < count; c++){
            row[c]=constraint[c].value;
            col[c]=constraint[c].colno;
        }
        add_constraintex(_lp, count, row, col, constr_type, rh);
        delete[] row;
        delete[] col;
    }
    
    STSolver::STSolver(Reachability::ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth) : printer(printer), _query(query), _net(net){
        if(depth == 0){
            _siphonDepth = _net._nplaces;
        } else {
            _siphonDepth = depth;
        }
        
        _m0 = _net._initialMarking;
        _nPlaceVariables = (_net._nplaces * (_siphonDepth + 1));
        uint32_t nPostTransVariables = (_net._ntransitions * (_siphonDepth));
        _noTrap = _nPlaceVariables + nPostTransVariables + 1; // variable index
        _nCol = _noTrap; // number of columns
        _analysisTime=0;
        _buildTime=-1;
        _lpBuilt=false;
        _solved=false;
        constructPrePost(); // TODO: Refactor this out...
    }

    STSolver::~STSolver() {
        if(_lp != NULL) delete_lp(_lp);
    }

    /* 
     * (SUM(p in P) p^0) GE 1
     * 
     * for all (t in T)    
     *     for all (p in post(t))
     *         -p^0 + SUM(q in pre(t)) q^0 GE 0
     */ 
    int STSolver::CreateSiphonConstraints(){       
        // (SUM(p in P) p^0) >= 1
        std::vector<STVariable> variables;
        for(uint32_t p=0;p<_net._nplaces;p++){ 
            if(timeout()){ return TIMEOUT; }
            variables.push_back(STVariable(VarPlace(p,0), 1));
        }  
        MakeConstraint(variables, GE, 1);
        
        // for all (t in T) for all (p in post(t)) (-p^0 + SUM(q in pre(t)) q^0) GE 0
        for(uint32_t t=0; t<_net._ntransitions; t++){
            if(timeout()){ return TIMEOUT; }
            uint32_t finv = _net._transitions[t].outputs;
            uint32_t linv = _net._transitions[t+1].inputs; 
            for (; finv < linv; finv++) { // for all p in post(t)
                if(timeout()){ return TIMEOUT; }
                std::vector<STVariable> variables2;
                variables2.push_back(STVariable(VarPlace(_net._invariants[finv].place,0), -1)); // -p^0
                
                uint32_t finv2 = _net._transitions[t].inputs;
                uint32_t linv2 = _net._transitions[t].outputs;
                for(; finv2 < linv2; finv2++){ // SUM(q in pre(t))
                    if(timeout()){ return TIMEOUT; }
                    variables2.push_back(STVariable(VarPlace(_net._invariants[finv2].place,0), 1)); // q^0
                }
                MakeConstraint(variables2, GE, 0);
            }
        }
        return 0;
    }
    
    /* 
     * for all (p in P)
     *     -p^i+1 + p^i + SUM(t in post(p)) post_t^i LE card(post(p))
     * 
     *     for all (t in post(p)) 
     *         -p^i+1 + post_t^i GE 0 
     * 
     *     -p^i+1 + p^i GE 0
     */
    int STSolver::CreateStepConstraints(uint32_t i){  
        // for all (p in P) 
        // -p^i+1 + p^i + SUM(t in post(p) post_t^i LE card(post(p))         
        for(uint32_t p=0; p<_net._nplaces; p++){
            std::vector<STVariable> variables1;
            variables1.push_back(STVariable(VarPlace(p,(i+1)), -1)); // -p^i+1
            variables1.push_back(STVariable(VarPlace(p,i), 1));      // p^i
            
            // for all (t in post(p)) -p^i+1 + post_t^i GE 0
            for (uint32_t t = _places.get()[p].post; t < _places.get()[p + 1].pre; t++){ // for all t in post(p)
                if(timeout()){ return TIMEOUT; }
                variables1.push_back(STVariable(VarPostT(_transitions.get()[t],i), 1));  // post_t^i
                std::vector<STVariable> variables3;
                variables3.push_back(STVariable(VarPlace(p,(i+1)), -1));                 // -p^i+1
                variables3.push_back(STVariable(VarPostT(_transitions.get()[t],i), 1));  // post_t^i
                MakeConstraint(variables3, GE, 0);
            }
            MakeConstraint(variables1, LE, (_places.get()[p + 1].pre - _places.get()[p].post));        
            
            // -p^i+1 + p^i GE 0
            std::vector<STVariable> variables2;
            variables2.push_back(STVariable(VarPlace(p,(i+1)), -1)); // -p^i+1
            variables2.push_back(STVariable(VarPlace(p,i), 1));      // p^i
            MakeConstraint(variables2, GE, 0);
        }
        return 0;
    }
    
    /*
     * for all (t in T)
     *     -post_t^i + SUM(p in post(t)) p^i GE 0
     *      
     *     for all (p in post(t))
     *         -post_t + p^i LE 0
     */
    int STSolver::CreatePostVarDefinitions(uint32_t i){ 
        for(uint32_t t=0; t<_net._ntransitions; t++){
            if(timeout()){ return TIMEOUT; }
            // -post_t^i + SUM(p in post(t)) p^i GE 0
            std::vector<STVariable> variables;
            variables.push_back(STVariable(VarPostT(t,i), -1)); // -post_t^i
            
            uint32_t finv = _net._transitions[t].outputs;
            uint32_t linv = _net._transitions[t+1].inputs;
            for (; finv < linv; finv++) { // for all p in post(t)
                if(timeout()){ return TIMEOUT; }
                variables.push_back(STVariable(VarPlace(_net._invariants[finv].place,i), 1)); // p^i
            }
            MakeConstraint(variables, GE, 0);

            // for all (p in post(t)) -post_t + p^i LE 0
            finv = _net._transitions[t].outputs;
            linv = _net._transitions[t+1].inputs;
            for (; finv < linv; finv++) {  // for all p in post(t)
                if(timeout()){ return TIMEOUT; }
                std::vector<STVariable> variables;
                variables.push_back(STVariable(VarPostT(t,i), -1)); // -post_t^i
                variables.push_back(STVariable(VarPlace(_net._invariants[finv].place,i), 1)); // p^i
                MakeConstraint(variables, LE, 0);
            }   
        }
        return 0;
    }
    
    int STSolver::CreateNoTrapConstraints(){
        std::vector<STVariable> variables1;
        
        for(uint32_t p=0; p<_net._nplaces; p++){
            // for all p in P where M0(p) > 0: p^d - noTrap LE 0
            if(_m0[p] > 0){
                if(timeout()){ return TIMEOUT; }               
                std::vector<STVariable> variables2;
                variables2.push_back(STVariable(VarPlace(p,(_siphonDepth)), 1)); // p^d
                variables2.push_back(STVariable(_noTrap, -1)); // -noTrap
                MakeConstraint(variables2, LE, 0);   
            }
            
            // SUM(p in P) p^d + noTrap = SUM(p in P) p^d-1 
            variables1.push_back(STVariable(VarPlace(p,(_siphonDepth)),1)); // +p^d
            variables1.push_back(STVariable(VarPlace(p,(_siphonDepth-1)),-1)); // -p^d-1
        }
        variables1.push_back(STVariable(_noTrap, 1)); // +noTrap
        MakeConstraint(variables1, EQ, 0);
        return 0;
    }
  
    int STSolver::CreateFormula(){
        _ret = 0;
        for(auto &i : _net._invariants){
            if(i.tokens > 1){
                _ret = WEIGHTEDARCS; // the net has weighted arcs
                return _ret;
            }
        }        
        
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _ret = INHIBITORARCS;
                    return _ret;
                }
            }
        }
        
        
        if(_ret == 0){
            _lp = make_lp(0, _nCol);
            if(_lp == NULL) { std::cout<<"Could not construct new model ..."<<std::endl; return NUMFAILURE; }
        }
        if(_ret == 0){           
            
#ifdef DEBUG
            // Set variable names (not strictly necessary)
            for(uint32_t c=0; c<_nCol; c++){ 
                set_col_name(_lp, c+1, const_cast<char *> (VarName(c).c_str())); 
                //std::cout<<"name of col "<<c+1<<" is "<<VarName(c).c_str()<<std::endl;
            }       
#endif
            for (size_t i = 1; i <= _nCol-1; i++){ set_binary(_lp, i, TRUE); }
            set_int(_lp, _noTrap, TRUE);

            // Create constraints
            set_add_rowmode(_lp, TRUE);  

            if(CreateSiphonConstraints() == TIMEOUT)          // define initial siphon
                return TIMEOUT;
            for(uint32_t i=0; i<=_siphonDepth-1; i++){ 
                if(CreateStepConstraints(i) == TIMEOUT)       // maximal trap
                    return TIMEOUT;
                if(CreatePostVarDefinitions(i) == TIMEOUT)    // define post_t 
                    return TIMEOUT;
            }
            if(CreateNoTrapConstraints() == TIMEOUT)          // unmarked or not a trap
                return TIMEOUT;

            set_add_rowmode(_lp, FALSE);      

            // Bounds
            set_bounds(_lp, _noTrap, 0, _net._nplaces);  

            // Create objective
            std::vector<REAL> row = std::vector<REAL>(_nCol + 1);
            memset(row.data(), 0, sizeof (REAL) * _nCol + 1);
            for (size_t p = 0; p < _nCol; p++)
                row[1 + p] = 1;

            // Set objective
            set_obj_fn(_lp, row.data());

            // Minimize the objective
            set_minim(_lp);
            
        }
        return _ret;
    }
    
    int STSolver::Solve(uint32_t timelimit){
        _timelimit=timelimit;
        _start = std::chrono::high_resolution_clock::now();
        _ret = CreateFormula();
        if(_ret == 0){
            _lpBuilt=true;
            _buildTime=duration();
            set_break_at_first(_lp, TRUE);
            set_presolve(_lp, PRESOLVE_ROWS | PRESOLVE_COLS | PRESOLVE_LINDEP, get_presolveloops(_lp));
#ifdef DEBUG            
            write_LP(_lp, stdout);
#endif    
            if(!timeout()){
                timelimit = _timelimit-duration();
                if(timelimit > _timelimit) { timelimit = 1; }
                
                set_verbose(_lp, IMPORTANT);
                set_timeout(_lp, timelimit);
                _ret = solve(_lp);
                _solved=true;
            }
#ifdef DEBUG
            if(_ret == OPTIMAL) {
                /* a solution is calculated, print results */

                /* objective value */
                printf("Objective value: %f\n", get_objective(_lp));

                /* variable values */
                std::vector<REAL> row = std::vector<REAL>(_nCol + 1);
                get_variables(_lp, row.data());
                for(uint32_t j = 0; j < _nCol; j++)
                  printf("%s: %f\n", get_col_name(_lp, j + 1), row[j]);

              }
#endif
        }
        _analysisTime = duration();
        return _ret;
    }
    
    Reachability::ResultPrinter::Result STSolver::PrintResult(){
        if(_ret == INFEASIBLE){
            return printer.printResult(0, _query, Reachability::ResultPrinter::NotSatisfied);
        } else {
            return Reachability::ResultPrinter::Unknown;
        }
    }
    bool STSolver::timeout() const {
        return (duration() >= _timelimit);
    }
    uint32_t STSolver::duration() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
        return diff.count();
    }
    
    void STSolver::PrintStatistics(){
        std::cout<<std::endl;
        std::cout<<"Siphon-trap analysis is enabled."<<std::endl;
        std::cout<<"Places:      "<<_net._nplaces<<std::endl;
        std::cout<<"Transitions: "<<_net._ntransitions<<std::endl;
        std::cout<<"Arcs:        "<<_net._ninvariants<<std::endl;
        
        if(_lpBuilt){
            std::cout<<"LP was built in (seconds):   "<<_buildTime<<std::endl;
            std::cout<<"Variables before presolve:   "<<get_Norig_columns(_lp)<<std::endl;
            std::cout<<"Constraints before presolve: "<<get_Norig_rows(_lp)<<std::endl;
        }
        if(_solved){
            std::cout<<"Variables after presolve:    "<<get_Ncolumns(_lp)<<std::endl;
            std::cout<<"Constraints after presolve:  "<<get_Nrows(_lp)<<std::endl;
        }
        
        if(_ret == OPTIMAL) {
            std::cout<<"An optimal solution was obtained."<<std::endl;
        } else if(_ret == SUBOPTIMAL) {
            std::cout<<"The model is sub-optimal."<<std::endl;
        } else if(_ret == PRESOLVED){
            std::cout<<"The model could be solved by presolve."<<std::endl;
        } else if(_ret == INFEASIBLE){
            std::cout<<"The model is infeasible."<<std::endl;
        } else if(_ret == NOMEMORY){
            std::cout<<"Out of memory."<<std::endl;
        } else if(_ret == UNBOUNDED){
            std::cout<<"The model is unbounded."<<std::endl;
        } else if(_ret == DEGENERATE){
            std::cout<<"The model is degenerative."<<std::endl;
        } else if(_ret == NUMFAILURE){
            std::cout<<"Numerical failure encountered."<<std::endl;
        } else if(_ret == USERABORT){
            std::cout<<"The abort routine returned TRUE."<<std::endl;
        } else if(_ret == TIMEOUT){
            std::cout<<"A timeout occurred."<<std::endl;
        } else if(_ret == WEIGHTEDARCS) {
            std::cout<<"The net has weighed arcs."<<std::endl;
        } else if(_ret == INHIBITORARCS) {
            std::cout<<"The net has inhibitor arcs."<<std::endl;
        }
        if(_analysisTime < _timelimit){
            fprintf(stdout, "Siphon-trap analysis finished after %u seconds.\n", _analysisTime);
        } else {
            fprintf(stdout, "Siphon-trap analysis reached timeout (used %u seconds).\n", _analysisTime);
        }
    }
    
    std::string STSolver::VarName(uint32_t index){
        std::string name = "";
        if(index == _nCol-1){
            name = "NOTRAP";
        // place/transition index ^ siphon depth step
        }else if(index < _nPlaceVariables){ // place
            name = _net.placeNames()[(index % (_net._nplaces))] + "^" + std::to_string((uint32_t)floor(index / (_net._nplaces))); 
        } else{ // transition
            index=index-_nPlaceVariables;
            name = _net.transitionNames()[(index % (_net._ntransitions))]+ "^" + std::to_string(((uint32_t)floor(index / (_net._ntransitions))));
        }
        return name;
    }
    
    // TODO: Refactor this out... Copy paste from ReducingSuccessorGenerator.cpp
    // Also, we dont need the preset here.
    void STSolver::constructPrePost() {
        std::vector<std::pair<std::vector<uint32_t>, std::vector < uint32_t>>> tmp_places(_net._nplaces);
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                tmp_places[_net._invariants[finv].place].second.push_back(t);
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                tmp_places[_net._invariants[finv].place].first.push_back(t);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _transitions.reset(new uint32_t[ntrans]);

        _places.reset(new place_t[_net._nplaces + 1]);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            std::vector<uint32_t>& pre = tmp_places[p].first;
            std::vector<uint32_t>& post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _places.get()[p].pre = offset;
            offset += pre.size();
            _places.get()[p].post = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _transitions.get()[tn + _places.get()[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _transitions.get()[tn + _places.get()[p].post] = post[tn];
            }

        }
        assert(offset == ntrans);
        _places.get()[p].pre = offset;
        _places.get()[p].post = offset;
    }
}