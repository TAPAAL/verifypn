#include "PetriEngine/STSolver.h"

#include <glpk.h>
#include <cassert>

namespace PetriEngine {     
    #define VarPlace(p,i) (((_net._nplaces * (i)) + (p)) + 1)
    #define VarPostT(t,i) ((_nPlaceVariables + (_net._ntransitions * (i)) + (t)) + 1)
   
    // return values
    #define WEIGHTEDARCS             -6
    #define INHIBITORARCS            -7

    void STSolver::MakeConstraint(std::vector<STVariable> constraint, int constr_type, REAL rh){
        _row.resize(constraint.size()+1);
        _indir.resize(constraint.size()+1);
        for(size_t c = 0; c < constraint.size(); ++c){
            _row[c+1] = constraint[c].value;
            _indir[c+1] = constraint[c].colno;
        }

        glp_add_rows(_lp, 1);
        auto rowno = glp_get_num_rows(_lp);
        glp_set_mat_row(_lp, rowno, constraint.size(), _indir.data(), _row.data());
        glp_set_row_bnds(_lp, rowno, constr_type, rh, rh);
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
        if(_lp != nullptr) glp_delete_prob(_lp);
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
            if(timeout()){ return GLP_ETMLIM; }
            variables.push_back(STVariable(VarPlace(p,0), 1));
        }  
        MakeConstraint(variables, GLP_LO, 1);
        
        // for all (t in T) for all (p in post(t)) (-p^0 + SUM(q in pre(t)) q^0) GE 0
        for(uint32_t t=0; t<_net._ntransitions; t++){
            if(timeout()){ return GLP_ETMLIM; }
            uint32_t finv = _net._transitions[t].outputs;
            uint32_t linv = _net._transitions[t+1].inputs; 
            for (; finv < linv; finv++) { // for all p in post(t)
                if(timeout()){ return GLP_ETMLIM; }
                std::vector<STVariable> variables2;
                variables2.push_back(STVariable(VarPlace(_net._invariants[finv].place,0), -1)); // -p^0
                
                uint32_t finv2 = _net._transitions[t].inputs;
                uint32_t linv2 = _net._transitions[t].outputs;
                for(; finv2 < linv2; finv2++){ // SUM(q in pre(t))
                    if(timeout()){ return GLP_ETMLIM; }
                    variables2.push_back(STVariable(VarPlace(_net._invariants[finv2].place,0), 1)); // q^0
                }
                MakeConstraint(variables2, GLP_LO, 0);
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
                if(timeout()){ return GLP_ETMLIM; }
                variables1.push_back(STVariable(VarPostT(_transitions.get()[t],i), 1));  // post_t^i
                std::vector<STVariable> variables3;
                variables3.push_back(STVariable(VarPlace(p,(i+1)), -1));                 // -p^i+1
                variables3.push_back(STVariable(VarPostT(_transitions.get()[t],i), 1));  // post_t^i
                MakeConstraint(variables3, GLP_LO, 0);
            }
            MakeConstraint(variables1, GLP_UP, (_places.get()[p + 1].pre - _places.get()[p].post));
            
            // -p^i+1 + p^i GE 0
            std::vector<STVariable> variables2;
            variables2.push_back(STVariable(VarPlace(p,(i+1)), -1)); // -p^i+1
            variables2.push_back(STVariable(VarPlace(p,i), 1));      // p^i
            MakeConstraint(variables2, GLP_LO, 0);
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
            if(timeout()){ return GLP_ETMLIM; }
            // -post_t^i + SUM(p in post(t)) p^i GE 0
            std::vector<STVariable> variables;
            variables.push_back(STVariable(VarPostT(t,i), -1)); // -post_t^i
            
            uint32_t finv = _net._transitions[t].outputs;
            uint32_t linv = _net._transitions[t+1].inputs;
            for (; finv < linv; finv++) { // for all p in post(t)
                if(timeout()){ return GLP_ETMLIM; }
                variables.push_back(STVariable(VarPlace(_net._invariants[finv].place,i), 1)); // p^i
            }
            MakeConstraint(variables, GLP_LO, 0);

            // for all (p in post(t)) -post_t + p^i LE 0
            finv = _net._transitions[t].outputs;
            linv = _net._transitions[t+1].inputs;
            for (; finv < linv; finv++) {  // for all p in post(t)
                if(timeout()){ return GLP_ETMLIM; }
                std::vector<STVariable> variables;
                variables.push_back(STVariable(VarPostT(t,i), -1)); // -post_t^i
                variables.push_back(STVariable(VarPlace(_net._invariants[finv].place,i), 1)); // p^i
                MakeConstraint(variables, GLP_UP, 0);
            }   
        }
        return 0;
    }
    
    int STSolver::CreateNoTrapConstraints(){
        std::vector<STVariable> variables1;
        
        for(uint32_t p=0; p<_net._nplaces; p++){
            // for all p in P where M0(p) > 0: p^d - noTrap LE 0
            if(_m0[p] > 0){
                if(timeout()){ return GLP_ETMLIM; }
                std::vector<STVariable> variables2;
                variables2.push_back(STVariable(VarPlace(p,(_siphonDepth)), 1)); // p^d
                variables2.push_back(STVariable(_noTrap, -1)); // -noTrap
                MakeConstraint(variables2, GLP_UP, 0);
            }
            
            // SUM(p in P) p^d + noTrap = SUM(p in P) p^d-1 
            variables1.push_back(STVariable(VarPlace(p,(_siphonDepth)),1)); // +p^d
            variables1.push_back(STVariable(VarPlace(p,(_siphonDepth-1)),-1)); // -p^d-1
        }
        variables1.push_back(STVariable(_noTrap, 1)); // +noTrap
        MakeConstraint(variables1, GLP_FX, 0);
        return 0;
    }

    constexpr auto infty = std::numeric_limits<double>::infinity();
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
            _lp = glp_create_prob();
            glp_add_cols(_lp, _nCol+1);
            if(_lp == NULL) { std::cout<<"Could not construct new model ..."<<std::endl; return -1; }
        }
        if(_ret == 0){           
            
#ifdef DEBUG
            // Set variable names (not strictly necessary)
            for(uint32_t c=0; c<_nCol; c++){ 
                set_col_name(_lp, c+1, const_cast<char *> (VarName(c).c_str())); 
                //std::cout<<"name of col "<<c+1<<" is "<<VarName(c).c_str()<<std::endl;
            }       
#endif
            for (size_t i = 1; i <= _nCol-1; i++)
            {
                glp_set_col_kind(_lp, i, GLP_BV);
            }
            glp_set_col_kind(_lp, _noTrap, GLP_IV);

            // Create constraints
            if(CreateSiphonConstraints() == GLP_ETMLIM)          // define initial siphon
                return -2;
            for(uint32_t i=0; i<=_siphonDepth-1; i++){ 
                if(CreateStepConstraints(i) == GLP_ETMLIM)       // maximal trap
                    return -2;
                if(CreatePostVarDefinitions(i) == GLP_ETMLIM)    // define post_t
                    return -2;
            }
            if(CreateNoTrapConstraints() == GLP_ETMLIM)          // unmarked or not a trap
                return -2;

            // objective
            for(size_t i = 1; i <= _nCol; i++) {
                glp_set_obj_coef(_lp, i, 0);
                glp_set_col_bnds(_lp, i, GLP_LO, 0, infty);
            }


            // Bounds
            glp_set_col_bnds(_lp, _noTrap, GLP_LO, 0, _net._nplaces);
            glp_set_obj_dir(_lp, GLP_MIN);
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
            if(!timeout()){
                timelimit = _timelimit-duration();
                if(timelimit > _timelimit) { timelimit = 1; }

                auto stime = glp_time();
                glp_smcp settings;
                glp_init_smcp(&settings);
                settings.tm_lim = timelimit*1000;
                settings.presolve = GLP_OFF;
                settings.msg_lev = 1;
                auto result = glp_simplex(_lp, &settings);
                if(result == 0)
                {
                    auto status = glp_get_status(_lp);
                    if(status == GLP_OPT) {
                        glp_iocp isettings;
                        glp_init_iocp(&isettings);
                        isettings.tm_lim = std::max<int>(((double) timelimit * 1000) - (glp_time() - stime), 1);
                        isettings.msg_lev = 1;
                        isettings.presolve = GLP_OFF;
                        glp_intopt(_lp, &isettings);
                        _ret = glp_mip_status(_lp);
                    } else _ret = status;
                }
            }
        }
        _analysisTime = duration();
        return _ret;
    }
    
    Reachability::ResultPrinter::Result STSolver::PrintResult(){
        if(_ret == GLP_INFEAS || _ret == GLP_NOFEAS || _ret == GLP_UNDEF){
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
        }

        if(_ret == GLP_OPT) {
            std::cout<<"An optimal solution was obtained."<<std::endl;
        } else if(_ret == GLP_FEAS) {
            std::cout<<"The model is feasible."<<std::endl;
        } else if(_ret == GLP_UNDEF){
            std::cout<<"The model was undefined."<<std::endl;
        } else if(_ret == GLP_INFEAS){
            std::cout<<"The model is infeasible."<<std::endl;
        } else if(_ret == GLP_NOFEAS){
            std::cout<<"No feasible solution."<<std::endl;
        } else if(_ret == GLP_UNBND){
            std::cout<<"The model is unbounded."<<std::endl;
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
