/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *                     Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "PetriEngine/PQL/Simplifier.h"

#define RETURN(x) {_return_value = x; return;}

namespace PetriEngine { namespace PQL {

    Retval simplify(const std::shared_ptr<Condition> element, SimplificationContext& context) {
        Simplifier query_simplifier(context);
        Visitor::visit(query_simplifier, element);
        return std::move(query_simplifier.get_return_value());
    }

    AbstractProgramCollection_ptr mergeLps(std::vector<AbstractProgramCollection_ptr> &&lps) {
        if (lps.size() == 0) return nullptr;
        int j = 0;
        int i = lps.size() - 1;
        while (i > 0) {
            if (i <= j) j = 0;
            else {
                lps[j] = std::make_shared<MergeCollection>(lps[j], lps[i]);
                --i;
                ++j;
            }
        }
        return lps[0];
    }

    bool Simplifier::finalLpsImpossible(std::vector<AbstractProgramCollection_ptr>& final_lps){
        return finalLpsImpossibleAll(final_lps);
        for(int i = 0; i < final_lps.size(); i++){
            if(!final_lps[i]->satisfiable(_context)){
                return true;
            }
        }

        if(final_lps.size() == 1)
            return false;
    
        for(int i = 0; i < final_lps.size(); i++){
            for(int j = i + 1; j < final_lps.size(); j++){
                bool sat = false;
                for(auto& lp_i : final_lps[i]->AllProgs()){
                    for(auto& lp_j : final_lps[j]->AllProgs()){
                        if(!lp_i->isFinalImpossibleWith(lp_j, false, false, _context) ||
                            !lp_j->isFinalImpossibleWith(lp_i, false, false, _context))
                            {
                                sat = true;
                                break;
                            }
                    }
                    if(sat)
                        break;
                }
                if(!sat)
                    return true;
            }
        }
        return false;
    }

    bool Simplifier::finalLpsImpossibleAll(std::vector<AbstractProgramCollection_ptr>& final_lps){
        for(int i = 0; i < final_lps.size(); i++){
            if(!final_lps[i]->satisfiable(_context)){
                return true;
            }
        }

        if(final_lps.size() == 1)
            return false;

        std::vector<AbstractProgramCollection::NextProgramIterator> iters;

        for(int i = 0; i < final_lps.size(); i++){
            iters.push_back(AbstractProgramCollection::NextProgramIterator(final_lps[i].get()));
        }

        auto end = AbstractProgramCollection::NextProgramIterator();
        
        bool done = false;
        int n = final_lps.size();
        int lps_solved = 0;
        while(!done){
            /* produce all permutations */
            std::vector<uint32_t> perm(n);
            for(int i = 0; i < n; i++){
                perm[i] = i;
            }

            /* all current lps */
            std::vector<LinearProgram*> progs;
            for(int l = 0; l < n; l++){
                progs.push_back(*iters[l]);
                std::cout << progs[l]->size() << "\n";
            }
            bool sat = false;
            do{
                if(!progs[0]->isFinalImpossibleWithN(perm, progs, false, false, _context)){
                    sat = true;
                    break;
                }
                lps_solved += 1;
            }while(std::next_permutation(perm.begin(), perm.end()));

            if(sat)
                return false;

            int i = n - 1;
            while(1){
                iters[i]++;
                if(iters[i] == end){
                    iters[i] = AbstractProgramCollection::NextProgramIterator(final_lps[i].get());
                }else{
                    break;
                }
                i--;
                if(i < 0){
                    done = true;
                    break;
                }
            }
        }

        return true;
    }


    bool Simplifier::nextLpsImpossible(std::vector<AbstractProgramCollection_ptr>& next_lps, std::vector<AbstractProgramCollection_ptr>& final_lps, bool is_invariant, bool is_or){
        /*const bool strictNext = !_context.isDeadlocked();
        for(int i = 0; i < next_lps.size(); i++){
            if(is_invariant){
                bool nmore = false;
                bool nsat = false;
                next_lps[i]->reset();
                do{
                    nextProgram pn = next_lps[i]->get_next_program();
                    SingleProgram* sn = dynamic_cast<SingleProgram*>(pn.prog.get());
                    if(!sn || !sn->getProgram().isNStepsImpossible(1, strictNext, _context)){
                        nsat = true;
                    }
                    nmore = pn.hasmore;
                }while(nmore && !nsat);
                next_lps[i]->reset();
                if(!nsat){
                    std::cout << "strict next impossible\n";
                    return true;
                }
            }else{
                if(!next_lps[i]->satisfiable(_context)){
                    return true;
                }
            }
        }

        if(next_lps.size() == 0 || final_lps.size() == 0 || !is_invariant) 
            return false;

        for(int j = 0; j < final_lps.size(); j++){
            // checks if lp holds in initial marking, and if it does skips it 
            bool fmore = false;
            bool fsat = false;
            final_lps[j]->reset();
            do{
                nextProgram pf = final_lps[j]->get_next_program();
                SingleProgram* sf = dynamic_cast<SingleProgram*>(pf.prog.get());
                if(!sf || !sf->getProgram().isNStepsImpossible(0, true, _context)){
                    fsat = true;
                }
                fmore = pf.hasmore;
            }while(fmore && !fsat);
            final_lps[j]->reset();
            if(fsat)
                continue;

            for(int i = 0; i < next_lps.size(); i++){
                next_lps[i]->reset();
                final_lps[j]->reset();
                bool hasmore = false;
                bool sat = false;
                // the lps i/j may be union/mergecollections of many programs
                do{
                    nextProgram np_i = next_lps[i]->get_next_program();
                    bool submore = false;
                    do{
                        nextProgram np_j = final_lps[j]->get_next_program();
                        SingleProgram* s_i = dynamic_cast<SingleProgram*>(np_i.prog.get());
                        SingleProgram* s_j = dynamic_cast<SingleProgram*>(np_j.prog.get());
                        if(s_i && s_j){
                            LinearProgram lp_i = s_i->getProgram();
                            LinearProgram lp_j = s_j->getProgram();
                            if(!lp_i.isFinalImpossibleWith(lp_j, true, strictNext, _context)){ //&&
                               //!lp_j.isFinalImpossibleWith(lp_i, _context)){
                                sat = true;
                            }
                        }else{
                            // shouldn't happen, but in here for safety for now
                            std::cout << "# NOT A SINGLE PROGRAM\n";
                            sat = true;
                        }
                        submore = np_j.hasmore;
                    }while(submore && !sat);
                    hasmore = np_i.hasmore;
                    // if we found a satisfied combined lp, then (i, j) cannot give us a conclusive answer
                    // reset and move to next combination
                    if(sat){
                        hasmore = false;
                        next_lps[i]->reset();
                        final_lps[j]->reset();
                    }
                }while(hasmore);

                if(!sat){
                    return true;
                }
            }
        }*/
        return false;
    }



    AbstractProgramCollection_ptr createGlobalUnion(AbstractProgramCollection_ptr &global_lp, std::vector<AbstractProgramCollection_ptr> &&nonglobal_lps) {
        if(global_lp == nullptr){
            if(nonglobal_lps.size() == 0){
                return nullptr;
            }else{
                return std::make_shared<UnionCollection>(std::move(nonglobal_lps));
            }
        }

        if(nonglobal_lps.size() == 0){
            return global_lp;
        }
        
        std::vector<AbstractProgramCollection_ptr> merges;

        for(int i = 0; i < nonglobal_lps.size(); i++){
            merges.emplace_back(std::make_shared<MergeCollection>(global_lp, nonglobal_lps[i]));
        }

        return std::make_shared<UnionCollection>(std::move(merges));
    }

    Retval Simplifier::simplify_or(const LogicalCondition *element) {
        std::vector<Condition_ptr> conditions;
        std::vector<AbstractProgramCollection_ptr> lps;
        std::vector<AbstractProgramCollection_ptr> unquantified_neglpsv, global_neglpsv, final_neglpsv, next_neglpsv, nonglobal_neglpsv;

        const bool same_context = op_parent_negated == _context.negated();
        const bool parent_final = operator_parent == LPOP::FINAL;
        const bool parent_global = operator_parent == LPOP::GLOBAL;
        const bool neg_is_invariant = (same_context && parent_final) || (!same_context && parent_global);

        const auto local_parent = operator_parent;

        for (const auto &c: element->getOperands()) {
            operator_found = LPOP::NONE;
            int32_t pre_operators = operators;
            if(!neg_is_invariant)
                operator_parent = LPOP::OTHER;
            Visitor::visit(this, c);
            operator_parent = local_parent;

            auto r = std::move(_return_value);
            assert(r.neglps);
            assert(r.lps);

            if (r.formula->isTriviallyTrue()) {
                operator_found = LPOP::OTHER;
                return Retval(BooleanCondition::TRUE_CONSTANT);
            } else if (r.formula->isTriviallyFalse()) {
                continue;
            }

            conditions.push_back(r.formula);

            lps.emplace_back(r.lps);

            if( ( operators - pre_operators ) > 1){
                nonglobal_neglpsv.emplace_back(r.neglps);
            }else {
                switch(operator_found){
                    case LPOP::NONE:
                        if(neg_is_invariant){
                            global_neglpsv.emplace_back(r.neglps);
                        }else{
                            unquantified_neglpsv.emplace_back(r.neglps);
                        }
                        break;
                    case LPOP::FINAL:
                        global_neglpsv.emplace_back(r.neglps);
                        break;
                    case LPOP::GLOBAL:
                        final_neglpsv.emplace_back(r.neglps);
                        break;
                    case LPOP::UNTIL:
                        final_neglpsv.emplace_back(r.neglps);
                        break;
                    case LPOP::NEXT:
                        next_neglpsv.emplace_back(r.neglps);
                        break;
                    default:
                        nonglobal_neglpsv.emplace_back(r.neglps);
                }
            }   
        }
       
        operator_found = LPOP::OTHER;
        if (conditions.size() == 0) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        }

        nonglobal_neglpsv.clear();
        if(!_context.rules().F_rule){
            final_neglpsv.clear();
        }
        if(!_context.rules().G_rule){
            std::move(global_neglpsv.begin(), global_neglpsv.end(), std::back_inserter(nonglobal_neglpsv));
            global_neglpsv.clear();
        }
        if(!_context.rules().X_rule){
            next_neglpsv.clear();
        }

        if(unquantified_neglpsv.size() > 0){
            auto uq_neglps = mergeLps(std::move(unquantified_neglpsv));
            nonglobal_neglpsv.emplace_back(uq_neglps);
        }

       
        auto global_neglp = mergeLps(std::move(global_neglpsv));
        auto neglps = createGlobalUnion(global_neglp, std::move(nonglobal_neglpsv));

        if(neglps == nullptr){
            neglps = std::make_shared<SingleProgram>();
        }

        if(final_neglpsv.size() > 0){
            if(global_neglp){
                for(int i = 0; i < final_neglpsv.size(); i++){
                    final_neglpsv[i] = std::make_shared<MergeCollection>(global_neglp, final_neglpsv[i]);
                }
            }
            if(finalLpsImpossible(final_neglpsv)){
                return Retval(BooleanCondition::TRUE_CONSTANT); 
            }
        }

        if(next_neglpsv.size() > 0){
            if(global_neglp){
                for(int i = 0; i < next_neglpsv.size(); i++){
                    next_neglpsv[i] = std::make_shared<MergeCollection>(global_neglp, next_neglpsv[i]);
                }
            }
            const bool next_is_invariant = neg_is_invariant || (operator_parent == LPOP::NONE);
            if(nextLpsImpossible(next_neglpsv, final_neglpsv, next_is_invariant, true)){
                return Retval(BooleanCondition::TRUE_CONSTANT); 
            }
        }

        if(lps.size() == 0){
            lps.emplace_back(std::make_shared<SingleProgram>());
        }
        
        try {
            if (!_context.timeout() && !neglps->satisfiable(_context)) {
                return Retval(BooleanCondition::TRUE_CONSTANT);
            }
        }
        catch (std::bad_alloc &e) {
            // we are out of memory, deal with it.
            std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
        }

        // Lets try to see if the r1 AND r2 can ever be false at the same time
        // If not, then we know that r1 || r2 must be true.
        // we check this by checking if !r1 && !r2 is unsat
        //std::cout << "leaving or\n";
        return Retval(
                makeOr(conditions),
                std::make_shared<UnionCollection>(std::move(lps)),
                std::move(neglps));
    }

    
    Retval Simplifier::simplify_and(const LogicalCondition *element) {
        std::vector<Condition_ptr> conditions;
        std::vector<AbstractProgramCollection_ptr> global_lpsv, final_lpsv, next_lpsv, opfree_lpsv, nonglobal_lpsv;
        std::vector<AbstractProgramCollection_ptr> neglps;

        const bool same_context = op_parent_negated == _context.negated();
        const bool parent_final = operator_parent == LPOP::FINAL;
        const bool parent_global = operator_parent == LPOP::GLOBAL;
        const bool is_invariant = (same_context && parent_global) || (!same_context && parent_final);
        const auto local_parent = operator_parent;

        auto unique_operator = LPOP::NULLT;

        for (auto &c: element->getOperands()) {
            operator_found = LPOP::NONE;
            int32_t pre_operators = operators;
            if(!is_invariant)
                operator_parent = LPOP::OTHER;
            Visitor::visit(this, c);
            operator_parent = local_parent;

            if(unique_operator == LPOP::NULLT){
                unique_operator = operator_found;
            }else{
                if(operator_found != unique_operator){
                    unique_operator = LPOP::OTHER;
                }
            }

            auto r = std::move(_return_value);
            if (r.formula->isTriviallyFalse()) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            } else if (r.formula->isTriviallyTrue()) {
                continue;
            }

            conditions.push_back(r.formula);

            if( ( operators - pre_operators ) > 1){
                nonglobal_lpsv.emplace_back(r.lps);
            }else {
                switch(operator_found){
                    case LPOP::NONE:
                        if(is_invariant){
                            global_lpsv.emplace_back(r.lps);
                        }else{
                            opfree_lpsv.emplace_back(r.lps);
                        }
                        break;
                    case LPOP::GLOBAL:
                        global_lpsv.emplace_back(r.lps);
                        break;
                    case LPOP::FINAL:
                        final_lpsv.emplace_back(r.lps);
                        break;
                    case LPOP::UNTIL:
                        final_lpsv.emplace_back(r.lps);
                        break;
                    case LPOP::NEXT:
                        next_lpsv.emplace_back(r.lps);
                        break;
                    default:
                        nonglobal_lpsv.emplace_back(r.lps);
                }
            }   
            neglps.emplace_back(r.neglps);
        }

        if(unique_operator == LPOP::NULLT || unique_operator == LPOP::OTHER){
            operator_found == LPOP::OTHER;
        }
        
        if (conditions.size() == 0) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        }
        nonglobal_lpsv.clear();
        if(!_context.rules().F_rule){
            final_lpsv.clear();
        }
        if(!_context.rules().G_rule){
            std::move(global_lpsv.begin(), global_lpsv.end(), std::back_inserter(nonglobal_lpsv));
            global_lpsv.clear();
        }
        if(!_context.rules().X_rule){
            next_lpsv.clear();
        }

        if(opfree_lpsv.size() > 0){
            auto uq_lps = mergeLps(std::move(opfree_lpsv));
            nonglobal_lpsv.emplace_back(uq_lps);
        }

        auto global_lp = mergeLps(std::move(global_lpsv));
        auto lps = createGlobalUnion(global_lp, std::move(nonglobal_lpsv));
        
        if(lps == nullptr){
            lps = std::make_shared<SingleProgram>();
        }

        if(final_lpsv.size() > 0){
            if(global_lp){
                for(int i = 0; i < final_lpsv.size(); i++){
                    final_lpsv[i] = std::make_shared<MergeCollection>(global_lp, final_lpsv[i]);
                }
            }
            if(finalLpsImpossible(final_lpsv)){
                return Retval(BooleanCondition::FALSE_CONSTANT); 
            }
        }

        if(next_lpsv.size() > 0){
            if(global_lp){
                for(int i = 0; i < next_lpsv.size(); i++){
                    next_lpsv[i] = std::make_shared<MergeCollection>(global_lp, next_lpsv[i]);
                }
            }
            const bool next_is_invariant = is_invariant || (operator_parent == LPOP::NONE);
            if(nextLpsImpossible(next_lpsv, final_lpsv, next_is_invariant)){
                return Retval(BooleanCondition::FALSE_CONSTANT); 
            }
        }

        if(neglps.size() == 0){
            neglps.emplace_back(std::make_shared<SingleProgram>());
        }

        try {
            if (!_context.timeout() && !lps->satisfiable(_context)) {
                return Retval(BooleanCondition::FALSE_CONSTANT);
            }
        }
        catch (std::bad_alloc &e) {
            // we are out of memory, deal with it.
            std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
        }

        
        // Lets try to see if the r1 AND r2 can ever be false at the same time
        // If not, then we know that r1 || r2 must be true.
        // we check this by checking if !r1 && !r2 is unsat
        return Retval(
                makeAnd(conditions),
                std::move(lps),
                std::make_shared<UnionCollection>(std::move(neglps)));
    }

    /************ Auxiliary functions for quantifier simplification ***********/

    Retval Simplifier::simplify_AG(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<AGCondition>(r.formula));
        }
    }

    Retval Simplifier::simplify_AF(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<AFCondition>(r.formula));
        }
    }

    Retval Simplifier::simplify_AX(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(std::make_shared<DeadlockCondition>());
        } else {
            return Retval(std::make_shared<AXCondition>(r.formula));
        }
    }

    Retval Simplifier::simplify_EG(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<EGCondition>(r.formula));
        }
    }

    Retval Simplifier::simplify_EF(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<EFCondition>(r.formula));
        }
    }

    Retval Simplifier::simplify_EX(Retval &r) {
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(std::make_shared<NotCondition>(
                    std::make_shared<DeadlockCondition>()));
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<EXCondition>(r.formula));
        }
    }

    /*Retval Simplifier::simplify_global_quantifier(Retval &r) {
        //std::cout << "triv true: " << r.formula->isTriviallyTrue() << "\n";
        //std::cout << "triv false: " << r.formula->isTriviallyFalse() << "\n";
        operator_found = LPOP::GLOBAL;
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<GCondition>(r.formula), r.lps, r.neglps);
        }
    }*/

    template<typename Quantifier>
    Retval Simplifier::simplify_simple_quantifier(Retval &r) {
        //std::cout << "other quantifier\n";
        static_assert(std::is_base_of_v<SimpleQuantifierCondition, Quantifier>);
        operator_found = LPOP::OTHER;
        //std::cout << "triv true: " << r.formula->isTriviallyTrue() << "\n";
        //std::cout << "triv false: " << r.formula->isTriviallyFalse() << "\n";
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<Quantifier>(r.formula), r.lps, r.neglps);
        }
    }

    bool Simplifier::isNextImpossible(AbstractProgramCollection_ptr next_lps, bool strict){
        /*if(strict){
            bool nmore = false;
            bool nsat = false;
            next_lps->reset();
            do{
                nextProgram pn = next_lps->get_next_program();
                SingleProgram* sn = dynamic_cast<SingleProgram*>(pn.prog.get());
                if(!sn || !sn->getProgram().isNStepsImpossible(1, true, _context)){
                    nsat = true;
                }
                nmore = pn.hasmore;
            }while(nmore && !nsat);
            next_lps->reset();
            if(!nsat){
                return true;
            }
        }else{
            if(!next_lps->satisfiable(_context)){
                return true;
            }
        }*/
        
        return false;
    }

    template<>
    Retval Simplifier::simplify_simple_quantifier<XCondition>(Retval &r, bool strict){
        operator_found = LPOP::NEXT;
        //std::cout << "has next\n";
        if(strict){
            if(isNextImpossible(r.neglps, true)){
                return Retval(BooleanCondition::TRUE_CONSTANT);
            }
            else if(isNextImpossible(r.lps, true)){
                return Retval(BooleanCondition::FALSE_CONSTANT);
            }
        }

        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        }
        return Retval(std::make_shared<XCondition>(r.formula), r.lps, r.neglps);
    }

    template<>
    Retval Simplifier::simplify_simple_quantifier<GCondition>(Retval &r){
        operator_found = LPOP::GLOBAL;
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<GCondition>(r.formula), r.lps, r.neglps);
        }
    }

    template<>
    Retval Simplifier::simplify_simple_quantifier<FCondition>(Retval &r){
        operator_found = LPOP::FINAL;
        if (r.formula->isTriviallyTrue() || !r.neglps->satisfiable(_context)) {
            return Retval(BooleanCondition::TRUE_CONSTANT);
        } else if (r.formula->isTriviallyFalse() || !r.lps->satisfiable(_context)) {
            return Retval(BooleanCondition::FALSE_CONSTANT);
        } else {
            return Retval(std::make_shared<FCondition>(r.formula), r.lps, r.neglps);
        }
    }

    Member memberForPathPlace(size_t p, int cur_path, const SimplificationContext &context) {
        std::vector<int64_t> row(( context.net()->numberOfTransitions() + context.net()->numberOfPlaces() ) * context.numPaths(), 0);
        row.shrink_to_fit();
        int variable_offset = cur_path * ( context.net()->numberOfTransitions() + context.net()->numberOfPlaces() );
        for (size_t t = 0; t < context.net()->numberOfTransitions(); t++) {
            row[t + variable_offset]  = context.net()->outArc(t, p);
            row[t + variable_offset] -= context.net()->inArc(p, t);
        }
        row[context.net()->numberOfTransitions() + variable_offset + p] = 1.0;
        return Member(std::move(row), 0);
    }

    Member memberForPlace(size_t p, const SimplificationContext &context) {
        std::vector<int64_t> row(context.net()->numberOfTransitions() + context.net()->numberOfPlaces(), 0);
        row.shrink_to_fit();
        for (size_t t = 0; t < context.net()->numberOfTransitions(); t++) {
            row[t] = context.net()->outArc(t, p);
            row[t] -= context.net()->inArc(p, t);
        }
        row[context.net()->numberOfTransitions() + p] = 1.0;
        return Member(std::move(row), 0);
    }

    /********** Constraint Visitor **********/
    Member constraint(const Expr *element, const SimplificationContext &context) {
        ConstraintVisitor visitor(context);
        Visitor::visit(visitor, element);
        return visitor.get_return_value();
    }

    void ConstraintVisitor::_commutative_cons(const CommutativeExpr *element, int _constant, const std::function<void(Member &a, Member b)>& op) {
        Member res;
        bool first = true;
        if (element->constant() != _constant || (element->operands() == 0 && element->places().empty())) {
            first = false;
            res = Member(element->constant());
        }

        for (auto &i: element->places()) {
            if (first) res = memberForPathPlace(i.first, _current_path, _context);
            else op(res, memberForPathPlace(i.first, _current_path, _context));
            first = false;
        }

        for (auto &e: element->expressions()) {
            Visitor::visit(*this, e);
            if (first) {
                res = _return_value;
            }
            else {
                op(res, _return_value);
            }
            first = false;
        }
        _return_value = res;
    }

    void ConstraintVisitor::_accept(const UnfoldedIdentifierExpr *element) {
        _return_value = memberForPathPlace(element->offset(), _current_path, _context);
    }

    void ConstraintVisitor::_accept(const PlusExpr *element) {
        _commutative_cons(element, 0, [](auto &a, auto b) { a += b; });
    }

    void ConstraintVisitor::_accept(const SubtractExpr *element) {
        Visitor::visit(*this, (*element)[0]);
        Member res = _return_value;
        for (size_t i = 1; i < element->operands(); ++i) {
            Visitor::visit(*this, (*element)[i]);
            res -= _return_value;
        }
        _return_value = res;
    }

    void ConstraintVisitor::_accept(const MultiplyExpr *element) {
        _commutative_cons(element, 1, [](auto &a, auto b) { a *= b; });
    }

    void ConstraintVisitor::_accept(const MinusExpr *element) {
        Member neg(-1);
        Visitor::visit(*this, (*element)[0]);
        _return_value = _return_value *= neg;
    }

    void ConstraintVisitor::_accept(const LiteralExpr *element) {
        _return_value = Member(element->value());
    }

    void ConstraintVisitor::_accept(const IdentifierExpr *element) {
        Visitor::visit(*this, element->compiled());
    }

    void ConstraintVisitor::_accept(const PathSelectExpr *element) {
        if(element->offset() >= _context.numPaths())
            _return_value = Member(0, false);
        else{
            _current_path = element->offset();
            Visitor::visit(*this, element->child());
            _current_path = 0;
        }
    }

    /******* Simplifier accepts ********/

    void Simplifier::_accept(const NotCondition *element) {
        //std::cout << "negating\n";
        _context.negate();
        Visitor::visit(this, element->getCond());
        _context.negate();
        // No return, since it will already be set by visit call
    }

    void Simplifier::_accept(const AndCondition *element) {
        if (_context.timeout()) {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<NotCondition>(makeAnd(element->getOperands()))))
            } else {
                RETURN(Retval(makeAnd(element->getOperands())))
            }
        }

        if (_context.negated()) {
            RETURN(simplify_or(element))
        } else {
            RETURN(simplify_and(element))
        }
    }

    void Simplifier::_accept(const OrCondition *element) {
        if (_context.timeout()) {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<NotCondition>(makeOr(element->getOperands()))))
            } else {
                RETURN(Retval(makeOr(element->getOperands())))
            }
        }
        if (_context.negated()) {
            RETURN(simplify_and(element))
        } else {
            RETURN(simplify_or(element))
        }
    }

    void Simplifier::_accept(const LessThanCondition *element) {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);
        AbstractProgramCollection_ptr lps, neglps;
        if (!_context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
            // test for trivial comparison
            Trivial eval = _context.negated() ? m1 >= m2 : m1 < m2;
            if (eval != Trivial::Indeterminate) {
                RETURN(Retval(BooleanCondition::getShared(eval == Trivial::True)))
            } else { // if no trivial case
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                      (_context.negated() ? Simplification::OP_GE
                                                                          : Simplification::OP_LT));
                neglps = std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                         (!_context.negated() ? Simplification::OP_GE
                                                                              : Simplification::OP_LT));
            }
        } else {
            lps = std::make_shared<SingleProgram>();
            neglps = std::make_shared<SingleProgram>();
        }

        if (!_context.timeout() && !lps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::FALSE_CONSTANT))
        } else if (!_context.timeout() && !neglps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::TRUE_CONSTANT))
        } else {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<LessThanOrEqualCondition>((*element)[1], (*element)[0]),
                                      std::move(lps), std::move(neglps)))
            } else {
                RETURN(Retval(std::make_shared<LessThanCondition>((*element)[0], (*element)[1]), std::move(lps),
                                      std::move(neglps)))
            }
        }
    }

    void Simplifier::_accept(const LessThanOrEqualCondition *element) {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);

        AbstractProgramCollection_ptr lps, neglps;
        if (!_context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
            // test for trivial comparison
            Trivial eval = _context.negated() ? m1 > m2 : m1 <= m2;
            if (eval != Trivial::Indeterminate) {
                RETURN(Retval(BooleanCondition::getShared(eval == Trivial::True)))
            } else { // if no trivial case
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                      (_context.negated() ? Simplification::OP_GT
                                                                          : Simplification::OP_LE));
                neglps = std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                         (_context.negated() ? Simplification::OP_LE
                                                                             : Simplification::OP_GT));
            }
        } else {
            lps = std::make_shared<SingleProgram>();
            neglps = std::make_shared<SingleProgram>();
        }

        assert(lps);
        assert(neglps);

        if (!_context.timeout() && !neglps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::TRUE_CONSTANT))
        } else if (!_context.timeout() && !lps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::FALSE_CONSTANT))
        } else {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<LessThanCondition>(
                        (*element)[1], (*element)[0]), std::move(lps), std::move(neglps)))
            } else {
                RETURN(Retval(std::make_shared<LessThanOrEqualCondition>(
                        (*element)[0], (*element)[1]), std::move(lps), std::move(neglps)))
            }
        }
    }

    void Simplifier::_accept(const EqualCondition *element) {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);
        std::shared_ptr<AbstractProgramCollection> lps, neglps;
        if (!_context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
            if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2)) {
                RETURN(Retval(BooleanCondition::getShared(
                        _context.negated() ? (m1.constant() != m2.constant()) : (m1.constant() == m2.constant()))))
            } else {
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                neglps =
                        std::make_shared<UnionCollection>(
                                std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                                Simplification::OP_GT),
                                std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                                Simplification::OP_LT));
                Member m3 = m2;
                lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m3), constant, Simplification::OP_EQ);

                if (_context.negated()) lps.swap(neglps);
            }
        } else {
            lps = std::make_shared<SingleProgram>();
            neglps = std::make_shared<SingleProgram>();
        }

        if (!_context.timeout() && !lps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::FALSE_CONSTANT))
        } else if (!_context.timeout() && !neglps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::TRUE_CONSTANT))
        } else {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<NotEqualCondition>((*element)[0], (*element)[1]), std::move(lps),
                                      std::move(neglps)))
            } else {
                RETURN(Retval(std::make_shared<EqualCondition>((*element)[0], (*element)[1]), std::move(lps),
                                      std::move(neglps)))
            }
        }
    }

    void Simplifier::_accept(const NotEqualCondition *element) {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);
        std::shared_ptr<AbstractProgramCollection> lps, neglps;
        if (!_context.timeout() && m1.canAnalyze() && m2.canAnalyze()) {
            if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2)) {
                RETURN(Retval(std::make_shared<BooleanCondition>(
                        _context.negated() ? (m1.constant() == m2.constant()) : (m1.constant() != m2.constant()))))
            } else {
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                lps =
                        std::make_shared<UnionCollection>(
                                std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                                Simplification::OP_GT),
                                std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                                Simplification::OP_LT));
                Member m3 = m2;
                neglps = std::make_shared<SingleProgram>(_context.cache(), std::move(m3), constant,
                                                         Simplification::OP_EQ);

                if (_context.negated()) lps.swap(neglps);
            }
        } else {
            lps = std::make_shared<SingleProgram>();
            neglps = std::make_shared<SingleProgram>();
        }
        if (!_context.timeout() && !lps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::FALSE_CONSTANT))
        } else if (!_context.timeout() && !neglps->satisfiable(_context)) {
            RETURN(Retval(BooleanCondition::TRUE_CONSTANT))
        } else {
            if (_context.negated()) {
                RETURN(Retval(std::make_shared<EqualCondition>((*element)[0], (*element)[1]), std::move(lps),
                                      std::move(neglps)))
            } else {
                RETURN(Retval(std::make_shared<NotEqualCondition>((*element)[0], (*element)[1]), std::move(lps),
                                      std::move(neglps)))
            }
        }
    }

    void Simplifier::_accept(const DeadlockCondition *element) {
        if (_context.negated()) {
            RETURN(Retval(std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK)))
        } else {
            RETURN(Retval(DeadlockCondition::DEADLOCK))
        }
    }

    void Simplifier::_accept(const CompareConjunction *element) {
        // this case is unclear to me in the hyperltl construction
        assert(_context.numPaths() == 1);
        if(_context.numPaths() != 1){
            std::cerr << "CompareConjunction simplification for HyperLTL is not implemented \n";
            RETURN(Retval(std::make_shared<CompareConjunction>(*element, _context.negated())))
        }
        if (_context.timeout()) {
            RETURN(Retval(std::make_shared<CompareConjunction>(*element, _context.negated())))
        }
        std::vector<AbstractProgramCollection_ptr> neglps, lpsv;
        auto neg = _context.negated() != element->isNegated();
        std::vector<CompareConjunction::cons_t> nconstraints;
        for (auto &c: element->constraints()) {
            nconstraints.push_back(c);
            if (c._lower != 0 /*&& !_context.timeout()*/ ) {
                auto m2 = memberForPlace(c._place, _context);
                Member m1(c._lower);
                // test for trivial comparison
                Trivial eval = m1 <= m2;
                if (eval != Trivial::Indeterminate) {
                    if (eval == Trivial::False) {
                        RETURN(Retval(BooleanCondition::getShared(neg)))
                    } else
                        nconstraints.back()._lower = 0;
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    auto lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                              Simplification::OP_LE);
                    auto nlp = std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                               Simplification::OP_GT);
                    lpsv.push_back(lp);
                    neglps.push_back(nlp);
                }
            }

            if (c._upper != std::numeric_limits<uint32_t>::max() /*&& !_context.timeout()*/) {
                auto m1 = memberForPlace(c._place, _context);
                Member m2(c._upper);
                // test for trivial comparison
                Trivial eval = m1 <= m2;
                if (eval != Trivial::Indeterminate) {
                    if (eval == Trivial::False) {
                        RETURN(Retval(BooleanCondition::getShared(neg)))
                    } else
                        nconstraints.back()._upper = std::numeric_limits<uint32_t>::max();
                } else { // if no trivial case
                    int constant = m2.constant() - m1.constant();
                    m1 -= m2;
                    m2 = m1;
                    auto lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                              Simplification::OP_LE);
                    auto nlp = std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                               Simplification::OP_GT);
                    lpsv.push_back(lp);
                    neglps.push_back(nlp);
                }
            }

            assert(nconstraints.size() > 0);
            if (nconstraints.back()._lower == 0 && nconstraints.back()._upper == std::numeric_limits<uint32_t>::max())
                nconstraints.pop_back();

            assert(nconstraints.size() <= neglps.size() * 2);
        }

        auto lps = mergeLps(std::move(lpsv));

        if (lps == nullptr && !_context.timeout()) {
            RETURN(Retval(BooleanCondition::getShared(!neg)))
        }

        try {
            if (!_context.timeout() && lps && !lps->satisfiable(_context)) {
                RETURN(Retval(BooleanCondition::getShared(neg)))
            }
        }
        catch (std::bad_alloc &e) {
            // we are out of memory, deal with it.
            std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
        }
        // Lets try to see if the r1 AND r2 can ever be false at the same time
        // If not, then we know that r1 || r2 must be true.
        // we check this by checking if !r1 && !r2 is unsat
        try {
            // remove trivial rules from neglp
            int ncnt = neglps.size() - 1;
            for (int i = nconstraints.size() - 1; i >= 0; --i) {
                if (_context.timeout()) break;
                assert(ncnt >= 0);
                size_t cnt = 0;
                auto &c = nconstraints[i];
                if (c._lower != 0) ++cnt;
                if (c._upper != std::numeric_limits<uint32_t>::max()) ++cnt;
                for (size_t j = 0; j < cnt; ++j) {
                    assert(ncnt >= 0);
                    if (!neglps[ncnt]->satisfiable(_context)) {
                        if (j == 1 || c._upper == std::numeric_limits<uint32_t>::max())
                            c._lower = 0;
                        else if (j == 0)
                            c._upper = std::numeric_limits<uint32_t>::max();
                        neglps.erase(neglps.begin() + ncnt);
                    }
                    if (c._upper == std::numeric_limits<uint32_t>::max() && c._lower == 0)
                        nconstraints.erase(nconstraints.begin() + i);
                    --ncnt;
                }
            }
        }
        catch (std::bad_alloc &e) {
            // we are out of memory, deal with it.
            std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
        }
        if (nconstraints.size() == 0) {
            RETURN(Retval(BooleanCondition::getShared(!neg)))
        }


        Condition_ptr rc = [&]() -> Condition_ptr {
            if (nconstraints.size() == 1) {
                auto &c = nconstraints[0];
                auto id = std::make_shared<UnfoldedIdentifierExpr>(c._name, c._place);
                auto ll = std::make_shared<LiteralExpr>(c._lower);
                auto lu = std::make_shared<LiteralExpr>(c._upper);
                if (c._lower == c._upper) {
                    if (c._lower != 0)
                        if (neg) {
                            return std::make_shared<NotEqualCondition>(id, lu);
                        } else return std::make_shared<EqualCondition>(id, lu);
                    else if (neg) return std::make_shared<LessThanCondition>(lu, id);
                    else return std::make_shared<LessThanOrEqualCondition>(id, lu);
                } else {
                    if (c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max()) {
                        if (neg)
                            return makeOr(std::make_shared<LessThanCondition>(id, ll),
                                          std::make_shared<LessThanCondition>(lu, id));
                        else
                            return makeAnd(std::make_shared<LessThanOrEqualCondition>(ll, id),
                                           std::make_shared<LessThanOrEqualCondition>(id, lu));
                    } else if (c._lower != 0) {
                        if (neg) return std::make_shared<LessThanCondition>(id, ll);
                        else return std::make_shared<LessThanOrEqualCondition>(ll, id);
                    } else {
                        if (neg) return std::make_shared<LessThanCondition>(lu, id);
                        else return std::make_shared<LessThanOrEqualCondition>(id, lu);
                    }
                }
            } else {
                return std::make_shared<CompareConjunction>(std::move(nconstraints),
                                                            _context.negated() != element->isNegated());
            }
        }();


        if (!neg) {
            RETURN(Retval(rc, std::move(lps), std::make_shared<UnionCollection>(std::move(neglps))))
        } else {
            RETURN(Retval(rc, std::make_shared<UnionCollection>(std::move(neglps)), std::move(lps)))
        }
    }

    void Simplifier::_accept(const UnfoldedUpperBoundsCondition *element) {
        std::vector<UnfoldedUpperBoundsCondition::place_t> next;
        std::vector<uint32_t> places;
        for (auto &p: element->places())
            places.push_back(p._place);
        const auto nplaces = element->places().size();
        const auto bounds = LinearProgram::bounds(_context, _context.getLpTimeout(), places);
        double offset = element->getOffset();
        for (size_t i = 0; i < nplaces; ++i) {
            if (bounds[i].first != 0 && !bounds[i].second)
                next.emplace_back(element->places()[i], bounds[i].first);
            if (bounds[i].second)
                offset += bounds[i].first;
        }
        if (bounds[nplaces].second) {
            next.clear();
            RETURN(Retval(std::make_shared<UnfoldedUpperBoundsCondition>
                                          (next, 0, bounds[nplaces].first + element->getOffset())))
        }
        RETURN(Retval(std::make_shared<UnfoldedUpperBoundsCondition>
                                      (next, bounds[nplaces].first - offset, offset)))
    }

    void Simplifier::_accept(const ControlCondition *condition) {
        Visitor::visit(this, condition->getCond());
        if(_return_value.formula->isTriviallyTrue() || _return_value.formula->isTriviallyFalse())
        {
            bool is_true = _return_value.formula->isTriviallyTrue() xor _context.negated();
            RETURN(Retval(is_true ?
                           Retval(BooleanCondition::TRUE_CONSTANT) :
                           Retval(BooleanCondition::FALSE_CONSTANT)))
        }
        else
        {
            RETURN(Retval(std::make_shared<ControlCondition>(_context.negated() ?
                                                             std::make_shared<NotCondition>(_return_value.formula) :
                                                             _return_value.formula
                )))
        }
    }

    void Simplifier::_accept(const EFCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AG(_return_value) : simplify_EF(_return_value))
    }

    void Simplifier::_accept(const EXCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AX(_return_value) : simplify_EX(_return_value))
    }

    void Simplifier::_accept(const AXCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EX(_return_value) : simplify_AX(_return_value))
    }

    void Simplifier::_accept(const AFCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EG(_return_value) : simplify_AF(_return_value))
    }

    void Simplifier::_accept(const EGCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AF(_return_value) : simplify_EG(_return_value))
    }

    void Simplifier::_accept(const AGCondition *condition) {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EF(_return_value) : simplify_AG(_return_value))
    }

    void Simplifier::_accept(const EUCondition *condition) {
        // cannot push negation any further
        bool neg = _context.negated();
        _context.setNegate(false);
        Visitor::visit(this, (*condition)[1]);
        Retval r2 = std::move(_return_value);
        if (r2.formula->isTriviallyTrue() || !r2.neglps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::FALSE_CONSTANT) :
                           Retval(BooleanCondition::TRUE_CONSTANT))
        } else if (r2.formula->isTriviallyFalse() || !r2.lps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::TRUE_CONSTANT) :
                           Retval(BooleanCondition::FALSE_CONSTANT))
        }
        Visitor::visit(this, (*condition)[0]);
        Retval r1 = std::move(_return_value);
        _context.setNegate(neg);

        if (_context.negated()) {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<EFCondition>(r2.formula))))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(r2.formula)))
            } else {
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<EUCondition>(r1.formula, r2.formula))))
            }
        } else {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<EFCondition>(r2.formula)))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(std::move(r2))
            } else {
                RETURN(Retval(std::make_shared<EUCondition>(r1.formula, r2.formula)))
            }
        }
    }

    void Simplifier::_accept(const AUCondition *condition) {
        // cannot push negation any further
        bool neg = _context.negated();
        _context.setNegate(false);
        Visitor::visit(this, condition->getCond2());
        Retval r2 = std::move(_return_value);
        if (r2.formula->isTriviallyTrue() || !r2.neglps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::FALSE_CONSTANT) :
                           Retval(BooleanCondition::TRUE_CONSTANT))
        } else if (r2.formula->isTriviallyFalse() || !r2.lps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::TRUE_CONSTANT) :
                           Retval(BooleanCondition::FALSE_CONSTANT))
        }

        Visitor::visit(this, condition->getCond1());
        Retval r1 = std::move(_return_value);
        _context.setNegate(neg);

        if (_context.negated()) {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<AFCondition>(r2.formula))))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(r2.formula)))
            } else {
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<AUCondition>(r1.formula, r2.formula))))
            }
        } else {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<AFCondition>(r2.formula)))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(std::move(r2))
            } else {
                RETURN(Retval(std::make_shared<AUCondition>(r1.formula, r2.formula)))
            }
        }
    }

    void Simplifier::_accept(const UntilCondition *condition) {
        bool neg = _context.negated();
        _context.setNegate(false);
        operators++;
        operator_parent = LPOP::OTHER;

        Visitor::visit(this, condition->getCond2());
        Retval r2 = std::move(_return_value);
        if (r2.formula->isTriviallyTrue() || !r2.neglps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::FALSE_CONSTANT) :
                           Retval(BooleanCondition::TRUE_CONSTANT))
        } else if (r2.formula->isTriviallyFalse() || !r2.lps->satisfiable(_context)) {
            _context.setNegate(neg);
            RETURN(neg ?
                           Retval(BooleanCondition::TRUE_CONSTANT) :
                           Retval(BooleanCondition::FALSE_CONSTANT))
        }
        operator_parent = LPOP::OTHER;
        Visitor::visit(this, condition->getCond1());
        Retval r1 = std::move(_return_value);

        _context.setNegate(neg);

        if (_context.negated()) {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<FCondition>(r2.formula))))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<NotCondition>(r2.formula)))
            } else {
                operator_parent = LPOP::UNTIL;
                RETURN(Retval(std::make_shared<NotCondition>(
                        std::make_shared<UntilCondition>(r1.formula, r2.formula)), r2.neglps, r2.lps))
            }
        } else {
            if (r1.formula->isTriviallyTrue() || !r1.neglps->satisfiable(_context)) {
                RETURN(Retval(std::make_shared<FCondition>(r2.formula)))
            } else if (r1.formula->isTriviallyFalse() || !r1.lps->satisfiable(_context)) {
                RETURN(std::move(r2))
            } else {
                operator_parent = LPOP::UNTIL;
                RETURN(Retval(std::make_shared<UntilCondition>(r1.formula, r2.formula), r2.lps, r2.neglps))
            }
        }
    }

    void Simplifier::_accept(const ECondition *condition) {
        assert(false);
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_simple_quantifier<ACondition>(_return_value)
                                         : simplify_simple_quantifier<ECondition>(_return_value))
    }

    void Simplifier::_accept(const ACondition *condition) {
        assert(false);
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_simple_quantifier<ECondition>(_return_value)
                                         : simplify_simple_quantifier<ACondition>(_return_value))
    }

    void Simplifier::_accept(const ExistPath* condition) {
        assert(false);
        Visitor::visit(this, condition->child());
    }

    void Simplifier::_accept(const AllPaths* condition) {
        assert(false);
    }

    void Simplifier::_accept(const FCondition *condition) {
        operators++;
        if(_context.negated()){
            operator_parent = LPOP::GLOBAL;
        }else{
            operator_parent = LPOP::FINAL;
        }
        op_parent_negated = _context.negated();
        Visitor::visit(this, condition->getCond());
        if(_context.negated()){
            //std::cout << "negated GCondition\n";
            auto r = simplify_simple_quantifier<GCondition>(_return_value);
            RETURN(std::move(r));
        }else{
            //std::cout << "FCondition\n";
            RETURN(simplify_simple_quantifier<FCondition>(_return_value));
        }
    }

    void Simplifier::_accept(const GCondition *condition) {
        operators++;
        if(_context.negated()){
            operator_parent = LPOP::FINAL;
        }else{
            operator_parent = LPOP::GLOBAL;
        }
        op_parent_negated = _context.negated();
        Visitor::visit(this, condition->getCond());

        if(_context.negated()){
            //std::cout << "negated FCondition\n";
            RETURN(simplify_simple_quantifier<FCondition>(_return_value));
        }else{
            //std::cout << "GCondition\n";
            auto r  = simplify_simple_quantifier<GCondition>(_return_value);
            RETURN(std::move(r))
        }
    }

    void Simplifier::_accept(const XCondition *condition) {
        operators++;
        operator_parent = LPOP::NEXT;
        int32_t pre_operators = operators;
        Visitor::visit(this, condition->getCond());
        const bool is_strict_next = (operator_parent = LPOP::NONE) && (operators == pre_operators) && (!_context.isDeadlocked());
        RETURN(simplify_simple_quantifier<XCondition>(_return_value, is_strict_next))
    }

    void Simplifier::_accept(const BooleanCondition *condition) {
        if (_context.negated()) {
            RETURN(Retval(BooleanCondition::getShared(!condition->value)))
        } else {
            RETURN(Retval(BooleanCondition::getShared(condition->value)))
        }
    }

    void Simplifier::_accept(const PathSelectCondition* condition) {
        if(condition->offset() != 0)
        {
            Visitor::visit(this, condition->child());
            if(_return_value.formula->isTriviallyFalse() || _return_value.formula->isTriviallyFalse())
                RETURN(_return_value.formula)
            else
                RETURN(Retval(std::make_shared<PathSelectCondition>(condition->name(), _return_value.formula, condition->offset())))
        }
        else
        {
            auto res = std::make_shared<PathSelectCondition>(condition->name(), condition->child(), condition->offset());
            if(_context.negated())
                RETURN(Retval(std::make_shared<NotCondition>(res)))
            else
                RETURN(Retval(res))
        }
    }
} }
