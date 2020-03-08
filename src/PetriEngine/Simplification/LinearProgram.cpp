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

        LinearProgram::LinearProgram(Vector* vec, int constant, op_t op, LPCache* factory){
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

        bool LinearProgram::isImpossible(const PQL::SimplificationContext& context, uint32_t solvetime) {
            bool use_ilp = true;
            auto net = context.net();
            auto m0 = context.marking();
            auto timeout = std::min(solvetime, context.getLpTimeout());

            if(_result != result_t::UKNOWN)
            {
                if(_result == result_t::IMPOSSIBLE)
                    return _result == result_t::IMPOSSIBLE;
            }

            if(_equations.size() == 0){
                return false;
            }
            const uint32_t nCol = net->numberOfTransitions();
            auto lp = glp_create_prob();
            int nRow = net->numberOfPlaces() + _equations.size();

            glp_add_cols(lp, nCol+1);
            glp_add_rows(lp, nRow+1);
            assert(lp);
            if (!lp) return false;
            std::vector<REAL> row = std::vector<REAL>(nCol + 1);
            std::vector<int32_t> indir(nCol + 1);
            for(size_t i = 0; i <= nCol; ++i)
                indir[i] = i;

            int rowno = 1;
            // restrict all places to contain 0+ tokens
            for (size_t p = 0; p < net->numberOfPlaces(); p++) {
                size_t l = 1;
                for (size_t t = 0; t < nCol; t++) {
                    row[l] = net->outArc(t, p) - net->inArc(p, t);
                    if(row[l] != 0){
                        indir[l] = t+1;
                        ++l;
                    }
                }
                glp_set_mat_row(lp, rowno, l-1, indir.data(), row.data());
                glp_set_row_bnds(lp, rowno, GLP_LO, (0.0 - (double)m0[p]), infty);
                ++rowno;
            }
            for(const auto& eq : _equations){
                auto l = eq.row->write_indir(row, indir);
                assert(!(std::isinf(eq.upper) && std::isinf(eq.lower)));
                glp_set_mat_row(lp, rowno, l-1, indir.data(), row.data());
                if(!std::isinf(eq.lower) && !std::isinf(eq.upper))
                {
                    if(eq.lower == eq.upper)
                        glp_set_row_bnds(lp, rowno, GLP_FX, eq.lower, eq.upper);
                    else
                    {
                        if(eq.lower > eq.upper)
                        {
                            _result = result_t::IMPOSSIBLE;
                            return true;
                        }
                        glp_set_row_bnds(lp, rowno, GLP_DB, eq.lower, eq.upper);
                    }
                }
                else if(std::isinf(eq.lower))
                    glp_set_row_bnds(lp, rowno, GLP_UP, -infty, eq.upper);
                else
                    glp_set_row_bnds(lp, rowno, GLP_LO, eq.lower, -infty);
                ++rowno;
            }

            // Set objective, kind and bounds
            for(size_t i = 1; i <= nCol; i++) {
                glp_set_obj_coef(lp, i, 0);
                glp_set_col_kind(lp, i, use_ilp ? GLP_IV :GLP_CV);
                glp_set_col_bnds(lp, i, GLP_LO, 0, infty);
            }

            // Minimize the objective
            glp_set_obj_dir(lp, GLP_MIN);
            auto stime = glp_time();
            glp_smcp settings;
            glp_init_smcp(&settings);
            settings.tm_lim = timeout*1000;
            settings.presolve = GLP_OFF;
            settings.msg_lev = 1;
            auto result = glp_simplex(lp, &settings);
            if (result == GLP_ETMLIM)
            {
                _result = result_t::UKNOWN;
                std::cerr << "glpk: timeout" << std::endl;
            }
            else if(result == 0)
            {
                auto status = glp_get_status(lp);
                if(status == GLP_OPT) {
                    glp_iocp iset;
                    glp_init_iocp(&iset);
                    iset.msg_lev = 1;
                    iset.tm_lim = std::max<uint32_t>(timeout*1000-(stime - glp_time()), 1);
                    iset.presolve = GLP_OFF;
                    auto ires = glp_intopt(lp, &iset);
                    if(ires == GLP_ETMLIM)
                    {
                        _result = result_t::UKNOWN;
                        std::cerr << "glpk mip: timeout" << std::endl;
                    }
                    else if(ires == 0)
                    {
                        auto ist = glp_mip_status(lp);
                        if(ist == GLP_OPT || ist == GLP_FEAS || ist == GLP_UNBND) {
                            _result = result_t::POSSIBLE;
                        }
                        else
                        {
                            _result = result_t::IMPOSSIBLE;
                        }

                    }
                }
                else if(status == GLP_FEAS || status == GLP_UNBND)
                {
                    _result = result_t::POSSIBLE;
                }
                else
                    _result = result_t::IMPOSSIBLE;
            }
            glp_delete_prob(lp);

            return _result == result_t::IMPOSSIBLE;
        }

        std::vector<std::pair<double,bool>> LinearProgram::bounds(const PQL::SimplificationContext& context, uint32_t solvetime, const std::vector<uint32_t>& places)
        {
            std::vector<std::pair<double,bool>> result(places.size() + 1, std::make_pair(std::numeric_limits<double>::infinity(), false));
            auto net = context.net();
            auto m0 = context.marking();
            auto timeout = solvetime;

            const uint32_t nCol = net->numberOfTransitions();
            const int nRow = net->numberOfPlaces();
            std::vector<REAL> row = std::vector<REAL>(nCol + 1);
            std::vector<int32_t> indir(nCol + 1);
            for(size_t i = 0; i <= nCol; ++i)
                indir[i] = i;

            auto* base_lp = glp_create_prob();
            {
                glp_add_cols(base_lp, nCol+1);
                glp_add_rows(base_lp, nRow+1);
                assert(base_lp);
                if (!base_lp) return result;

                int rowno = 1;

                // restrict all places to contain 0+ tokens
                for (size_t p = 0; p < net->numberOfPlaces(); ++p) {
                    memset(row.data(), 0, sizeof (REAL) * (nCol + 1));
                    size_t l = 1;
                    for (size_t t = 0; t < nCol; t++) {
                        row[l] = net->outArc(t, p) - net->inArc(p, t);
                        if(row[l] != 0){
                            indir[l] = t+1;
                            ++l;
                        }
                    }
                    glp_set_mat_row(base_lp, rowno, l-1, indir.data(), row.data());
                    glp_set_row_bnds(base_lp, rowno, GLP_LO, (0.0 - (double)m0[p]), infty);
                    ++rowno;
                }
            }

            // Minimize the objective
            glp_set_obj_dir(base_lp, GLP_MAX);

            glp_smcp settings;
            glp_init_smcp(&settings);
            settings.tm_lim = timeout*1000;
            settings.presolve = GLP_OFF;
            settings.msg_lev = 1;

            for(size_t it = 0; it <= places.size(); ++it)
            {
                // we want to start with the overall bound, most important
                // Spend time on rest after
                auto stime = glp_time();
                size_t pi;
                if(it == 0)
                    pi = places.size();
                else
                    pi = it - 1;

                if(context.timeout())
                {
                    glp_delete_prob(base_lp);
                    return result;
                }
                // Create objective
                memset(row.data(), 0, sizeof (REAL) * net->numberOfTransitions() + 1);
                double p0 = 0;
                bool all_le_zero = true;
                bool all_zero = true;
                if(pi < places.size())
                {
                    auto tp = places[pi];
                    p0 = m0[tp];
                    for (size_t t = 0; t < net->numberOfTransitions(); ++t)
                    {
                        row[1 + t] = net->outArc(t, tp) - net->inArc(tp, t);
                        all_le_zero &= row[1 + t] <= 0;
                        all_zero &= row[1 + t] == 0;
                    }
                }
                else
                {
                    for (size_t t = 0; t < net->numberOfTransitions(); ++t)
                    {
                        double cnt = 0;
                        for(auto tp : places)
                            cnt += net->outArc(t, tp) - net->inArc(tp, t);
                        row[1 + t] = cnt;
                        all_le_zero &= row[1 + t] <= 0;
                        all_zero &= row[1 + t] == 0;
                    }
                    for(auto tp : places)
                        p0 += m0[tp];
                }

                if(all_le_zero)
                {
                    result[pi].first = p0;
                    result[pi].second = all_zero;
                    if(pi == places.size())
                    {
                        glp_delete_prob(base_lp);
                        return result;
                    }
                    continue;
                }

                // Set objective

                auto* tmp_lp = glp_create_prob();
                glp_copy_prob(tmp_lp, base_lp, GLP_OFF);

                for(size_t i = 1; i <= nCol; i++) {
                    glp_set_obj_coef(tmp_lp, i, row[i]);
                    glp_set_col_kind(tmp_lp, i, GLP_IV);
                    glp_set_col_bnds(tmp_lp, i, GLP_LO, 0, infty);
                }

                auto rs = glp_simplex(tmp_lp, &settings);
                if (rs == GLP_ETMLIM)
                {
                    std::cerr << "glpk: timeout" << std::endl;
                }
                else if(rs == 0)
                {
                    auto status = glp_get_status(tmp_lp);
                    if(status == GLP_OPT) {
                        glp_iocp isettings;
                        glp_init_iocp(&isettings);
                        isettings.tm_lim = std::max<int>(((double) timeout * 1000) - (glp_time() - stime), 1);
                        isettings.msg_lev = 1;
                        isettings.presolve = GLP_OFF;
                        auto rs = glp_intopt(tmp_lp, &isettings);
                        if (rs == GLP_ETMLIM) {
                            std::cerr << "glpk mip: timeout" << std::endl;
                        } else if (rs == 0) {
                            auto status = glp_mip_status(tmp_lp);
                            if (status == GLP_OPT) {
                                result[pi].first = p0 + glp_mip_obj_val(tmp_lp);
                                result[pi].second = all_zero;
                            }
                            else if (status != GLP_UNBND && status != GLP_FEAS)
                            {
                                result[pi].first = p0;
                                result[pi].second = all_zero;
                            }
                        }
                    }
                    else if (status == GLP_INFEAS || status == GLP_NOFEAS || GLP_UNDEF)
                    {
                        result[pi].first = p0;
                        result[pi].second = all_zero;
                    }
                }
                glp_erase_prob(tmp_lp);
                if(pi == places.size() && result[places.size()].first >= p0)
                {
                    glp_erase_prob(base_lp);
                    return result;
                }
                if(pi == places.size() && places.size() == 1)
                {
                    result[0] = result[1];
                    glp_erase_prob(base_lp);
                    return result;
                }
            }
            glp_erase_prob(base_lp);
            return result;
        }


        void LinearProgram::make_union(const LinearProgram& other)
        {
            if(_result == IMPOSSIBLE || other._result == IMPOSSIBLE)
            {
                _result = IMPOSSIBLE;
                _equations.clear();
                assert(false);
                return;
            }

            auto it1 = _equations.begin();
            auto it2 = other._equations.begin();

            while(it1 != _equations.end() && it2 != other._equations.end())
            {
                if(it1->row < it2->row)
                {
                    ++it1;
                }
                else if(it2->row < it1->row)
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
                    /*if(n.upper < n.lower)
                    {
                        _result = result_t::IMPOSSIBLE;
                        _equations.clear();
                        return;
                    }*/
                    ++it1;
                    ++it2;
                }
            }

            if(it2 != other._equations.end())
                _equations.insert(_equations.end(), it2, other._equations.end());
        }
    }
}

