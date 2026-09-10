/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

#include "PetriEngine/PQL/Contexts.h"

#include <iostream>

namespace PetriEngine {
    namespace PQL {

        uint32_t AnalysisContext::resolve_trace_name(const std::string& s, bool create)
        {
            uint32_t id = _trace_names.size();
            auto [it, inserted] = _trace_names.emplace(std::make_pair(s,id));
            if (!inserted && create)
                throw base_error("Trace identifier ", s, " already existed.");
            if (inserted && !create)
                throw base_error("Trace identifier ", s, " does not exist, but is used as a prefix in the query.");
            return it->second;
        }

        bool ColoredAnalysisContext::resolvePlace(const shared_const_string& place, std::function<void(const shared_const_string&)>&& fn)
        {
            auto it = _coloredPlaceNames.find(place);
            if (it != _coloredPlaceNames.end()) {
                for (auto& [_, name] : it->second)
                    fn(name);
                return true;
            }
            return false;
        }

        bool ColoredAnalysisContext::resolveTransition(const shared_const_string& transition, std::function<void(const shared_const_string)>&& fn)
        {
            auto it = _coloredTransitionNames.find(transition);
            if (it != _coloredTransitionNames.end()) {
                for (auto& e : it->second)
                    fn(e);
                return true;
            }
            return false;
        }


        AnalysisContext::ResolutionResult AnalysisContext::resolve(const shared_const_string& identifier, bool place)
        {
            ResolutionResult result;
            result.offset = -1;
            result.success = false;
            auto& map = place ? _placeNames : _transitionNames;
            auto it = map.find(identifier);
            if (it != map.end()) {
                result.offset = (int) it->second;
                result.success = true;
                return result;
            }
            return result;
        }

        uint32_t SimplificationContext::getLpTimeout() const
        {
            return _lpTimeout;
        }

        uint32_t SimplificationContext::getPotencyTimeout() const
        {
            return _potencyTimeout;
        }

        uint32_t SimplificationContext::getPrintLevel() const 
        {
            return _lpPrintLevel;
        }

        double SimplificationContext::getReductionTime()
        {
            // duration in seconds
            auto end = std::chrono::high_resolution_clock::now();
            return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
        }

        glp_prob* SimplificationContext::makeBaseLP() const
        {
            if (_base_lp == nullptr)
                _base_lp = buildBase();
            if (_base_lp == nullptr)
                return nullptr;
            auto* tmp_lp = glp_create_prob();
            glp_copy_prob(tmp_lp, _base_lp, GLP_OFF);
            return tmp_lp;
        }

        glp_prob* SimplificationContext::buildBase() const
        {
            constexpr auto infty = std::numeric_limits<double>::infinity();
            if (timeout())
                return nullptr;

            auto* lp = glp_create_prob();
            if (lp == nullptr)
                return lp;

            const uint32_t nCol = getNumBaseVariables();
            const uint32_t nRow = getNumBaseConstraints();
            std::vector<int32_t> indir(std::max<uint32_t>(nCol, nRow) + 1);

            glp_add_cols(lp, nCol + 1);
            glp_add_rows(lp, nRow + 1);
            {
                std::vector<double> col = std::vector<double>(nRow + 1);
                for (size_t t = 0; t < _net->numberOfTransitions(); ++t) {
                    auto pre = _net->preset(t);
                    auto post = _net->postset(t);
                    size_t l = 1;
                    while (pre.first != pre.second ||
                           post.first != post.second) {
                        if (pre.first == pre.second || (post.first != post.second && post.first->place < pre.first->place)) {
                            col[l] = post.first->tokens;
                            indir[l] = post.first->place + 1;
                            ++post.first;
                        }
                        else if (post.first == post.second || (pre.first != pre.second && pre.first->place < post.first->place)) {
                            if (!pre.first->inhibitor)
                                col[l] = -(double) pre.first->tokens;
                            else
                                col[l] = 0;
                            indir[l] = pre.first->place + 1;
                            ++pre.first;
                        }
                        else {
                            assert(pre.first->place == post.first->place);
                            if (!pre.first->inhibitor)
                                col[l] = (double) post.first->tokens - (double) pre.first->tokens;
                            else
                                col[l] = (double) post.first->tokens;
                            indir[l] = pre.first->place + 1;
                            ++pre.first;
                            ++post.first;
                        }
                        ++l;
                    }
                    //glp_set_mat_col(lp, t + 1, l - 1, indir.data(), col.data());

                    addAllPathConstraint(lp, t, l, indir, col);

                    if (timeout()) {
                        std::cerr << "glpk: construction timeout" << std::endl;
                        glp_delete_prob(lp);
                        return nullptr;
                    }
                }
            }
            int rowno = 1;
            std::vector<int> ind(2);
            std::vector<double> one = {0, 1.0};
            for(size_t path = 0; path < static_cast<size_t>(_num_paths); path++){
                const int variable_offset = path * ( _net->numberOfTransitions() + _net->numberOfPlaces() );
                const int constraint_offset = path * _net->numberOfPlaces();
                for (size_t p = 0; p < _net->numberOfPlaces(); p++) {
                    const int colno = 1 + p +  _net->numberOfTransitions() + variable_offset;
                    ind[1] = p+1 + constraint_offset;
                    glp_set_mat_col(lp, colno, 1, ind.data(), one.data());
                    glp_set_row_bnds(lp, rowno, GLP_LO, 0, infty);
                    ++rowno;

                    glp_set_col_bnds(lp, colno, GLP_FX, (double) _marking[p], 0);
                    glp_set_obj_coef(lp, colno, 0);
                    glp_set_col_kind(lp, colno, GLP_IV);

                    if (timeout()) {
                        std::cerr << "glpk: construction timeout" << std::endl;
                        glp_delete_prob(lp);
                        return nullptr;
                    }
                }
            }
        
            return lp;
        }

   
    void SimplificationContext::addAllPathConstraint(glp_prob* lp, size_t t, size_t l, int32_t* ind_data, double* col_data) const{
            std::vector<int32_t> indir_path(l);
            for(size_t path = 0; path < static_cast<size_t>(_num_paths); path++){
                const int variable_offset = path * ( _net->numberOfTransitions() + _net->numberOfPlaces() );
                for(int i = 1; i < l; i++){
                    indir_path[i] = ind_data[i] + path * _net->numberOfPlaces();
                }
                glp_set_mat_col(lp, t + 1 + variable_offset, l - 1, indir_path.data(), col_data);
            }
    }
    void SimplificationContext::addAllPathConstraint(glp_prob* lp, size_t t, size_t l, std::vector<int32_t>& ind, std::vector<double>& col) const{
        addAllPathConstraint(lp, t, l, ind.data(), col.data());
    }

    glp_prob* SimplificationContext::buildBaseFromMarking(std::vector<std::pair<std::vector<uint32_t>, double>>& setMarking) const
        {
            constexpr auto infty = std::numeric_limits<double>::infinity();
            if (timeout())
                return nullptr;

            auto* lp = glp_create_prob();
            if (lp == nullptr)
                return lp;

            const uint32_t nCol = _net->numberOfTransitions();
            const int nRow = setMarking.size();
            std::vector<int32_t> indir(std::max<uint32_t>(nCol, nRow) + 1);

            glp_add_cols(lp, nCol + 1);
            glp_add_rows(lp, nRow + 1);
            {
                std::vector<double> col = std::vector<double>(nRow + 1);
                for (size_t t = 0; t < _net->numberOfTransitions(); ++t) {
                    auto pre = _net->preset(t);
                    auto post = _net->postset(t);
                    size_t l = 1;
                    while (pre.first != pre.second ||
                           post.first != post.second) {
                        if (pre.first == pre.second || (post.first != post.second && post.first->place < pre.first->place)) {
                            col[l] = post.first->tokens;
                            indir[l] = post.first->place + 1;
                            ++post.first;
                        }
                        else if (post.first == post.second || (pre.first != pre.second && pre.first->place < post.first->place)) {
                            if (!pre.first->inhibitor)
                                col[l] = -(double) pre.first->tokens;
                            else
                                col[l] = 0;
                            indir[l] = pre.first->place + 1;
                            ++pre.first;
                        }
                        else {
                            assert(pre.first->place == post.first->place);
                            if (!pre.first->inhibitor)
                                col[l] = (double) post.first->tokens - (double) pre.first->tokens;
                            else
                                col[l] = (double) post.first->tokens;
                            indir[l] = pre.first->place + 1;
                            ++pre.first;
                            ++post.first;
                        }
                        ++l;
                    }
                    glp_set_mat_col(lp, t + 1, l - 1, indir.data(), col.data());
                    if (timeout()) {
                        std::cerr << "glpk: construction timeout" << std::endl;
                        glp_delete_prob(lp);
                        return nullptr;
                    }
                }
            }
            int rowno = 1;
            for (size_t p = 0; p < _net->numberOfPlaces(); p++) {
                if(setMarking[p].second >= 0){
                    glp_set_row_bnds(lp, rowno, GLP_LO, (0.0 - (double) setMarking[p].second), infty);
                }else{
                    glp_set_row_bnds(lp, rowno, GLP_FR, -infty, infty);
                }
                ++rowno;
                if (timeout()) {
                    std::cerr << "glpk: construction timeout" << std::endl;
                    glp_delete_prob(lp);
                    return nullptr;
                }
            }

            std::vector<double> col = std::vector<double>(nCol + 1);
            for(size_t s = _net->numberOfPlaces(); s < setMarking.size(); s++){
                if(setMarking[s].second < 0){
                    ++rowno;
                    continue;
                }
                size_t l = 1;
                for (size_t t = 0; t < _net->numberOfTransitions(); ++t) {
                    double coef = 0;
                    for(auto p: setMarking[s].first){
                        coef += _net->inArc(t, p);
                        coef -= _net->outArc(p, t);
                    }

                    if(coef != 0){
                        indir[l] = t + 1;
                        col[l] = coef;
                        l++;
                    }
                }
                glp_set_mat_row(lp, s + 1, l - 1, indir.data(), col.data());
                glp_set_row_bnds(lp, rowno, GLP_LO, (0.0 - (double) setMarking[s].second), infty);
                ++rowno;
            }

            return lp;
        }
    }
}