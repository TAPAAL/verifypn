#include <cassert>
#include <cmath>
#include <glpk.h>
#include <fstream>

#include "PetriEngine/Simplification/LinearProgram.h"
#include "PetriEngine/Simplification/LPCache.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    namespace Simplification {
        using REAL = double;
        LinearProgram::~LinearProgram() {
        }

        LinearProgram::LinearProgram(Vector* vec, int64_t constant, op_t op, LPCache* factory){
            // TODO fix memory-management here!
            equation_t c;
            switch(op)
            {
                case Simplification::OP_LT:
                    c.upper = constant - 1;
                    break;
                case Simplification::OP_GT:
                    c.lower = constant + 1;
                    break;
                case Simplification::OP_LE:
                    c.upper = constant;
                    break;
                case Simplification::OP_GE:
                    c.lower = constant;
                    break;
                case Simplification::OP_EQ:
                    c.lower = constant;
                    c.upper = constant;
                    break;
                default:
                    // We ignore this operator for now by not adding any equation.
                    // This is untested, however.
                    assert(false);
            }
            c.row = vec;
            vec->inc();
            _equations.push_back(c);
        }

        constexpr auto infty = std::numeric_limits<REAL>::infinity();

        std::string bound_or_inf(double bound){
            std::string inf = "inf";
            std::string ninf = "ninf";
            return (std::fabs(bound) != infty)? std::to_string(bound) : ((bound > 0)? inf : ninf);
        }

        void printConstraints(const PQL::SimplificationContext& context, glp_prob* lp){
            int nCols = glp_get_num_cols(lp);
            int nRows = glp_get_num_rows(lp);
            
            std::vector<int> indices(nCols + 1);
            std::vector<double> coef(nCols + 1);
            int l;

            for(int i = 1; i <= nRows; i++){
                l = glp_get_mat_row(lp, i, indices.data(), coef.data());
                std::cout << "Row " << i << "[len: " << l << "]: ";



                bool first_print = true;
                for(int j = 1; j <= l; j++){
                    if(!first_print) std::cout << " + ";
                    if((indices[j] - 1) < context.net()->numberOfTransitions()){
                        std::cout << coef[j] << "*" << *context.net()->transitionNames()[indices[j] - 1];
                    }else{
                        std::cout << coef[j] << "*" << glp_get_col_name(lp, indices[j]);
                    }
                    first_print = false;
                }

                if(first_print){
                    std::cout << "<empty-row>";
                }
                

                double up = glp_get_row_ub(lp, i);
                double lb = glp_get_row_lb(lp, i);
                switch(glp_get_row_type(lp, i)){
                    case GLP_FR:
                        std::cout << " is unconstraint";
                        break;
                    case GLP_LO:
                        std::cout << " >= " << bound_or_inf(lb);
                        break;
                    case GLP_UP:
                        std::cout << " <= " << bound_or_inf(up);
                        break;
                    case GLP_DB:
                        std::cout << " in [" << bound_or_inf(lb) << ", " << bound_or_inf(up) << "]";
                        break;
                    case GLP_FX:
                        std::cout << " = " << bound_or_inf(lb);
                        break;
                }
                std::cout << "\n";
            }
            
        }
        

        void printSolution(const PQL::SimplificationContext& context, glp_prob* lp, bool is_mip){
            if(context.getPrintLevel() == 0)
                return;

            if(context.getPrintLevel() > 1){
                printConstraints(context, lp);
            }
            
            int nCols = glp_get_num_cols(lp);
            bool first_print = true;
            if(is_mip){
                std::cout << "MIP Solution: ";
            }else{
                std::cout << "LP Solution: ";
            }
            for(int i = 1; i <= nCols;i++){
                double coef = (is_mip)? glp_mip_col_val(lp, i) : glp_get_col_prim(lp, i);
                if(coef != 0){
                    if(!first_print)
                        std::cout << " + ";
                    first_print = false;
                    if( i - 1 < context.net()->numberOfTransitions()){
                        std::cout << coef << "*" << *context.net()->transitionNames()[i - 1];
                    }else{
                        std::cout << coef << "*" << glp_get_col_name(lp, i);
                    }
                }
            }

            if(first_print)
                std::cout << "<empty>\n";
            else{
                std::cout << "\n";
            }
        }

        bool LinearProgram::isImpossible(const PQL::SimplificationContext& context, uint32_t solvetime) {
            bool use_ilp = true;
            auto net = context.net();


            if (_result != result_t::UKNOWN)
            {
                if (_result == result_t::IMPOSSIBLE)
                    return _result == result_t::IMPOSSIBLE;
            }

            if (_equations.size() == 0 || context.timeout()){
                return false;
            }

            if (context.markingOutOfBounds()) {  // the initial marking has too many tokens that exceed the int32_t limits
                return false;
            }

            const uint32_t nCol = net->numberOfTransitions();
            const uint32_t nRow = 2 * net->numberOfPlaces() + _equations.size();

            std::vector<REAL> row = std::vector<REAL>(nCol + 1);
            std::vector<int32_t> indir(std::max(nCol, nRow) + 1);
            for (size_t i = 0; i <= nCol; ++i)
                indir[i] = i;

            auto lp = context.makeBaseLP();
            if (lp == nullptr)
                return false;

            int rowno = 1 + net->numberOfPlaces();
            glp_add_rows(lp, _equations.size());
            for (const auto& eq : _equations) {
                auto l = eq.row->write_indir(row, indir);
                assert(!(std::isinf(eq.upper) && std::isinf(eq.lower)));
                glp_set_mat_row(lp, rowno, l-1, indir.data(), row.data());
                if (!std::isinf(eq.lower) && !std::isinf(eq.upper))
                {
                    if (eq.lower == eq.upper)
                        glp_set_row_bnds(lp, rowno, GLP_FX, eq.lower, eq.upper);
                    else
                    {
                        if (eq.lower > eq.upper)
                        {
                            _result = result_t::IMPOSSIBLE;
                            glp_delete_prob(lp);
                            return true;
                        }
                        glp_set_row_bnds(lp, rowno, GLP_DB, eq.lower, eq.upper);
                    }
                }
                else if (std::isinf(eq.lower))
                    glp_set_row_bnds(lp, rowno, GLP_UP, -infty, eq.upper);
                else
                    glp_set_row_bnds(lp, rowno, GLP_LO, eq.lower, -infty);
                ++rowno;

                if (context.timeout())
                {
                    // std::cerr << "glpk: construction timeout" << std::endl;
                    glp_delete_prob(lp);
                    return false;
                }
            }

            // Set objective, kind and bounds
            for (size_t i = 1; i <= nCol; i++) {
                glp_set_obj_coef(lp, i, 1);
                glp_set_col_kind(lp, i, use_ilp ? GLP_IV : GLP_CV);
                glp_set_col_bnds(lp, i, GLP_LO, 0, infty);
            }

            for (size_t p = 0; p < net->numberOfPlaces(); p++) {      
                    auto numColsLP = glp_get_num_cols(lp);
                    std::vector<double> inRow(1, 0);
                    std::vector<int> inIndices(1, 0);
                    
                    std::vector<double> outRow(1, 0);
                    std::vector<int> outIndices(1, 0);
                   

                    bool has_loop = false;
                    bool has_in   = false;
                    //bool requires_tokens = false;

                    //std::map<uint32_t, std::vector<uint32_t>> loops_by_weight;
                    //int min_pos_loop = 0;
                    uint32_t minimum_weight = UINT32_MAX;

                    for (size_t t = 0; t < net->numberOfTransitions(); t++) {
                        if(net->outArc(t, p) != 0 && net->inArc(p, t) != 0){
                            has_loop = true;
                            /*if(net->inArc(p, t) > context.marking()[p])
                                requires_tokens = true;
                            
                            if(net->outArc(t, p) > net->inArc(p, t) && net->inArc(p, t) < min_pos_loop)
                                min_pos_loop = net->inArc(p, t);*/

                            minimum_weight = std::min(minimum_weight, net->inArc(p, t));
                            
                            outRow.push_back(1/1000.0);
                            outIndices.push_back(t+1);
                        }
                        else{
                            if(net->inArc(p,t) != 0){
                                outRow.push_back(1/1000.0);
                                outIndices.push_back(t+1);
                            }
                            if(net->outArc(t,p) != 0){
                                has_in = true;
                                
                                inRow.push_back(net->outArc(t, p));
                                inIndices.push_back(t+1);
                            }
                        }
                    }

                    if(!has_loop || !has_in || context.marking()[p] >= minimum_weight){
                        //std::cout << "no loop applicable\n";
                        continue;
                    }
                    else{
                        //std::cout << "adding loop constraint for " << *net->placeNames()[p] <<"\n";
                    }

                    int idx = numColsLP + 1;
                    glp_add_cols(lp, 2);

                    for(int i = glp_get_num_cols(lp) - 1; i <= glp_get_num_cols(lp); i++){
                        glp_set_obj_coef(lp, i, 1);
                        glp_set_col_kind(lp, i, GLP_BV);
                        //glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0);
                    }

                    const int OR_P = glp_get_num_cols(lp) ;
                    std::string or_p_name = "NO_IMPLY_" + *net->placeNames()[p];
                    glp_set_col_name(lp, OR_P, or_p_name.c_str());
                    const int OR_Q = glp_get_num_cols(lp) - 1;
                    std::string or_q_name = "YES_IMPLY_" + *net->placeNames()[p];
                    glp_set_col_name(lp, OR_Q, or_q_name.c_str());
                  
                    const double needed_weight = (double) (minimum_weight - context.marking()[p]);
                    inRow.push_back(needed_weight);
                    inIndices.push_back(OR_P);

                    outRow.push_back(-10000.0);
                    outIndices.push_back(OR_Q);
                
                    glp_add_rows(lp, 3);

                    glp_set_mat_row(lp, rowno, inIndices.size() - 1, inIndices.data(), inRow.data());
                    glp_set_row_bnds(lp, rowno, GLP_LO, needed_weight, infty);
                    ++rowno;

                    glp_set_mat_row(lp, rowno, outIndices.size() - 1, outIndices.data(), outRow.data());
                    glp_set_row_bnds(lp, rowno, GLP_UP, 0.0, 0.0);
                    ++rowno;

                    double orRow[3];
                    int orIndices[3];
                    

                    orRow[1] = 1.0;
                    orRow[2] = 1.0;
                    orIndices[1] = OR_P;
                    orIndices[2] = OR_Q;

                    glp_set_mat_row(lp, rowno, 2, orIndices, orRow);
                    glp_set_row_bnds(lp, rowno, GLP_FX, 1.0, 1.0);
                    ++rowno;
            }
           
            // Minimize the objective
            glp_set_obj_dir(lp, GLP_MIN);
            auto stime = glp_time();
            glp_smcp settings;
            glp_init_smcp(&settings);
            auto timeout = std::min(solvetime, context.getLpTimeout()) * 1000;
            settings.tm_lim = timeout;
            settings.presolve = GLP_OFF;
            settings.msg_lev = 0;
            auto result = glp_simplex(lp, &settings);
            if (result == GLP_ETMLIM)
            {
                _result = result_t::UKNOWN;
                // std::cerr << "glpk: timeout" << std::endl;
            }
            else if (result == 0)
            {
                auto status = glp_get_status(lp);
                if (status == GLP_OPT)
                {
                    glp_iocp iset;
                    glp_init_iocp(&iset);
                    iset.msg_lev = 0;
                    iset.tol_int = 1e-10;
                    iset.tm_lim = std::min<uint32_t>(std::max<uint32_t>(timeout - (stime - glp_time()), 1), 1000);
                    iset.presolve = GLP_OFF;
                    auto ires = glp_intopt(lp, &iset);
                    if (ires == GLP_ETMLIM)
                    {
                        printSolution(context, lp, false);
                        _result = result_t::UKNOWN;
                        // std::cerr << "glpk mip: timeout" << std::endl;
                    }
                    else if (ires == 0)
                    {
                        auto ist = glp_mip_status(lp);
                        if (ist == GLP_OPT || ist == GLP_FEAS || ist == GLP_UNBND) {
                            printSolution(context, lp, true);
                            _result = result_t::POSSIBLE;
                        }
                        else
                        {
                            _result = result_t::IMPOSSIBLE;
                        }
                    }
                }
                else if (status == GLP_FEAS || status == GLP_UNBND)
                {
                    printSolution(context, lp, false);
                    _result = result_t::POSSIBLE;
                }
                else
                {
                    _result = result_t::IMPOSSIBLE;
                }
            }
            else if (result == GLP_ENOPFS || result == GLP_ENODFS || result == GLP_ENOFEAS)
            {
                _result = result_t::IMPOSSIBLE;
            }
            glp_delete_prob(lp);

            return _result == result_t::IMPOSSIBLE;
        }

        void LinearProgram::solvePotency(const PQL::SimplificationContext& context, std::vector<uint32_t>& potencies)
        {
            bool use_ilp = false; // No need to call the ILP solver
            auto net = context.net();

            if (_equations.size() == 0 || context.potencyTimeout())
                return;

            const uint32_t nCol = net->numberOfTransitions();
            assert(potencies.size() == nCol);
            const uint32_t nRow = net->numberOfPlaces() + _equations.size();

            std::vector<REAL> row = std::vector<REAL>(nCol + 1);
            std::vector<int32_t> indir(std::max(nCol, nRow) + 1);
            for (size_t i = 0; i <= nCol; ++i)
                indir[i] = i;

            auto lp = context.makeBaseLP();
            if (lp == nullptr)
                return;

            int rowno = 1 + net->numberOfPlaces();
            glp_add_rows(lp, _equations.size());
            for (const auto& eq : _equations)
            {
                auto l = eq.row->write_indir(row, indir);
                assert(!(std::isinf(eq.upper) && std::isinf(eq.lower)));
                glp_set_mat_row(lp, rowno, l-1, indir.data(), row.data());
                if (!std::isinf(eq.lower) && !std::isinf(eq.upper))
                {
                    if (eq.lower == eq.upper)
                        glp_set_row_bnds(lp, rowno, GLP_FX, eq.lower, eq.upper);
                    else
                    {
                        if (eq.lower > eq.upper)
                        {
                            glp_delete_prob(lp);
                            return;
                        }
                        glp_set_row_bnds(lp, rowno, GLP_DB, eq.lower, eq.upper);
                    }
                }
                else if (std::isinf(eq.lower))
                    glp_set_row_bnds(lp, rowno, GLP_UP, -infty, eq.upper);
                else
                    glp_set_row_bnds(lp, rowno, GLP_LO, eq.lower, -infty);
                ++rowno;

                if (context.potencyTimeout())
                {
                    glp_delete_prob(lp);
                    return;
                }
            }

            // Set objective, kind and bounds
            for (size_t i = 1; i <= nCol; i++)
            {
                glp_set_obj_coef(lp, i, 1); // We want to minimize the sum of the number of transitions fired
                glp_set_col_kind(lp, i, use_ilp ? GLP_IV : GLP_CV);
                glp_set_col_bnds(lp, i, GLP_LO, 0, infty);
            }

            // Minimize the objective
            glp_set_obj_dir(lp, GLP_MIN);
            glp_smcp settings;
            glp_init_smcp(&settings);
            auto timeout = context.getPotencyTimeout() * 1000;
            settings.tm_lim = timeout;
            settings.presolve = GLP_OFF;
            settings.msg_lev = 0;
            auto result = glp_simplex(lp, &settings);

            // if (result == GLP_ETMLIM): do nothing
            if (result == 0)
            {
                //TODO: using exact causes segfault sometimes as of GLPK 5.0 
                //! see bug report #2112388 (https://bugs.launchpad.net/tapaal/+bug/2112388)
                /*// We search for an exact solution
                result = glp_exact(lp, &settings);

                if (result == 0)
                {*/
                    for (size_t i = 1; i <= nCol; i++)
                    {
                        double col_struct = glp_get_col_prim(lp, i); // Get the value of the i'th column in the solution found
                        potencies[i - 1] += ceil(col_struct); // Round up because it represents the nb of transitions to fire
                        // TODO: we could aggregate with max instead of adding
                    }
                //}
            }

            glp_delete_prob(lp);
        }

        std::vector<std::pair<double,bool>> LinearProgram::bounds(const PQL::SimplificationContext& context, uint32_t solvetime, const std::vector<uint32_t>& places)
        {
            std::vector<std::pair<double,bool>> result(places.size() + 1, std::make_pair(std::numeric_limits<double>::infinity(), false));
            auto net = context.net();
            auto m0 = context.marking();
            auto timeout = std::min(solvetime, context.getLpTimeout());

            const uint32_t nCol = net->numberOfTransitions();
            std::vector<REAL> row = std::vector<REAL>(nCol + 1);


            glp_smcp settings;
            glp_init_smcp(&settings);
            settings.tm_lim = timeout*1000;
            settings.presolve = GLP_OFF;
            settings.msg_lev = 0;

            for (size_t it = 0; it <= places.size(); ++it)
            {
                // we want to start with the overall bound, most important
                // Spend time on rest after
                auto stime = glp_time();
                size_t pi;
                if (it == 0)
                    pi = places.size();
                else
                    pi = it - 1;

                if (context.timeout())
                {
                    return result;
                }
                // Create objective
                memset(row.data(), 0, sizeof (REAL) * net->numberOfTransitions() + 1);
                double p0 = 0;
                bool all_le_zero = true;
                bool all_zero = true;
                if (pi < places.size())
                {
                    auto tp = places[pi];
                    p0 = m0[tp];
                    for (size_t t = 0; t < net->numberOfTransitions(); ++t)
                    {
                        row[1 + t] = net->outArc(t, tp);
                        row[1 + t] -= net->inArc(tp, t);
                        all_le_zero &= row[1 + t] <= 0;
                        all_zero &= row[1 + t] == 0;
                    }
                }
                else
                {
                    for (size_t t = 0; t < net->numberOfTransitions(); ++t)
                    {
                        double cnt = 0;
                        for (auto tp : places) {
                            cnt += net->outArc(t, tp);
                            cnt -= net->inArc(tp, t);
                        }
                        row[1 + t] = cnt;
                        all_le_zero &= row[1 + t] <= 0;
                        all_zero &= row[1 + t] == 0;
                    }
                    for (auto tp : places)
                        p0 += m0[tp];
                }

                if (all_le_zero)
                {
                    result[pi].first = p0;
                    result[pi].second = all_zero;
                    if (pi == places.size())
                    {
                        return result;
                    }
                    continue;
                }

                // Set objective

                auto tmp_lp = context.makeBaseLP();
                if (tmp_lp == nullptr)
                    return result;

                // Max the objective
                glp_set_obj_dir(tmp_lp, GLP_MAX);

                for (size_t i = 1; i <= nCol; i++) {
                    glp_set_obj_coef(tmp_lp, i, row[i]);
                    glp_set_col_kind(tmp_lp, i, GLP_IV);
                    glp_set_col_bnds(tmp_lp, i, GLP_LO, 0, infty);
                }

                auto rs = glp_simplex(tmp_lp, &settings);
                if (rs == GLP_ETMLIM)
                {
                    //std::cerr << "glpk: timeout" << std::endl;
                }
                else if (rs == 0)
                {
                    auto status = glp_get_status(tmp_lp);
                    if (status == GLP_OPT) {
                        glp_iocp isettings;
                        glp_init_iocp(&isettings);
                        isettings.tm_lim = std::max<int>(((double) timeout * 1000) - (glp_time() - stime), 1);
                        isettings.msg_lev = 0;
                        isettings.presolve = GLP_OFF;
                        auto rs = glp_intopt(tmp_lp, &isettings);
                        if (rs == GLP_ETMLIM) {
                            //std::cerr << "glpk mip: timeout" << std::endl;
                        } else if (rs == 0) {
                            auto status = glp_mip_status(tmp_lp);
                            if (status == GLP_OPT) {
                                auto org = p0 + glp_mip_obj_val(tmp_lp);
                                result[pi].first = std::round(org);
                                result[pi].second = all_zero;
                            }
                            else if (status != GLP_UNBND && status != GLP_FEAS)
                            {
                                result[pi].first = p0;
                                result[pi].second = all_zero;
                            }
                        }
                    }
                    else if (status == GLP_INFEAS || status == GLP_NOFEAS || status == GLP_UNDEF)
                    {
                        result[pi].first = p0;
                        result[pi].second = all_zero;
                    }
                }
                else if (rs == GLP_ENOPFS || rs == GLP_ENODFS || rs == GLP_ENOFEAS)
                {
                    result[pi].first = p0;
                    result[pi].second = all_zero;
                }
                glp_erase_prob(tmp_lp);
                if (pi == places.size() && result[places.size()].first >= p0)
                {
                    return result;
                }
                if (pi == places.size() && places.size() == 1)
                {
                    result[0] = result[1];
                    return result;
                }
            }
            return result;
        }


        void LinearProgram::make_union(const LinearProgram& other)
        {
            if (_result == IMPOSSIBLE || other._result == IMPOSSIBLE)
            {
                _result = IMPOSSIBLE;
                _equations.clear();
                assert(false);
                return;
            }

            auto it1 = _equations.begin();
            auto it2 = other._equations.begin();
// std::cout << "In make_union: _equations.size()=" << _equations.size() << ", other._equations.size()=" << other._equations.size() << std::endl;
            while(it1 != _equations.end() && it2 != other._equations.end())
            {
                if (it1->row < it2->row)
                {
                    ++it1;
                }
                else if (it2->row < it1->row)
                {
                    it1 = _equations.insert(it1, *it2);
                    ++it2;
                    ++it1;
                }
                else
                {
                    equation_t& n = *it1;
                    n.lower = std::max(n.lower, it2->lower);
                    n.upper = std::min(n.upper, it2->upper);
                    /*if (n.upper < n.lower)
                    {
                        _result = result_t::IMPOSSIBLE;
                        _equations.clear();
                        return;
                    }*/
                    ++it1;
                    ++it2;
                }
            }

            if (it2 != other._equations.end())
                _equations.insert(_equations.end(), it2, other._equations.end());
// std::cout << "After make_union: _equations.size()=" << _equations.size() << std::endl;
        }
    }
}
