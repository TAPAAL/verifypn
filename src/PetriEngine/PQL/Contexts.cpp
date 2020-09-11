/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

#include "PetriEngine/PQL/Contexts.h"



namespace PetriEngine {
    namespace PQL {
       
        bool ColoredAnalysisContext::resolvePlace(const std::string& place, std::unordered_map<uint32_t, std::string>& out)
        {
            auto it = _coloredPlaceNames.find(place);
            if (it != _coloredPlaceNames.end()) {
                out = it->second;
                return true;
            }
            return false;
        }
        
        bool ColoredAnalysisContext::resolveTransition(const std::string& transition, std::vector<std::string>& out)
        {
            auto it = _coloredTransitionNames.find(transition);
            if (it != _coloredTransitionNames.end()) {
                out = it->second;
                return true;
            }
            return false;
        }

       
        AnalysisContext::ResolutionResult AnalysisContext::resolve(const std::string& identifier, bool place)
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

            const uint32_t nCol = _net->numberOfTransitions();
            const int nRow = _net->numberOfPlaces();
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
                            if(!pre.first->inhibitor)
                                col[l] = -(double) pre.first->tokens;
                            else
                                col[l] = 0;
                            indir[l] = pre.first->place + 1;
                            ++pre.first;
                        }
                        else {
                            assert(pre.first->place == post.first->place);
                            if(!pre.first->inhibitor)
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
                glp_set_row_bnds(lp, rowno, GLP_LO, (0.0 - (double) _marking[p]), infty);
                ++rowno;
                if (timeout()) {
                    std::cerr << "glpk: construction timeout" << std::endl;
                    glp_delete_prob(lp);
                    return nullptr;
                }
            }
            return lp;
        }
    }
}