#include "PetriEngine/Simplification/LinearPrograms.h"

#include <vector>

namespace PetriEngine {
    namespace Simplification {

        void AbstractProgramCollection::mergeBuffer::compile_program(){
            assert(!time_path->is_compiled);
            time_path->is_compiled = true;
            //std::cout << "CMP: [" << time_path->free_lps.size() << "," << time_path->final_lps.size() << "," << time_path->next_lps.size() << "," << time_path->global_lps.size() << "]\n";
            if(time_path->global_lps.size() == 0 && time_path->free_lps.size() < 2)
                return;
            LinearProgram global_program = LinearProgram();
            for(const auto& [tcx, lp] : time_path->global_lps)
                global_program.make_union(lp);
            
            LinearProgram free_program = LinearProgram();
            for(const auto& [tcx, lp] : time_path->free_lps)
                free_program.make_union(lp);

            if(!global_program.size() == 0){

                free_program.make_union(global_program);

                for(auto cur = time_path; cur != nullptr; cur = cur->next){
                    if(cur != time_path){
                        for(auto& [tcx, lp] : cur->free_lps)
                            lp.make_union(global_program);
                    }

                    for(auto& [tcx, lp] : cur->final_lps)
                        lp.make_union(global_program);
                    
                    for(auto& [tcx, lp] : cur->next_lps)
                        lp.make_union(global_program);
                }
                
                time_path->global_lps.clear();
                //time_path->global_lps.push_back({temporalContext(), std::make_shared<LinearProgram>(global_program)});
            }
            
            time_path->free_lps.clear();
            time_path->free_lps.push_back({temporalContext(), free_program}); 
        }

        void AbstractProgramCollection::mergeBuffer::apply_operator(operator_t update_op, int next_ops){
            switch(update_op){
                case operator_t::FREE:{
                    assert(next_ops == 0);
                    break;
                }
                case operator_t::G:{
                    //std::cout << "next ops " << next_ops << "\n";
                    assert(next_ops == 0);
                    auto& glps = time_path->global_lps;
                    auto& freelps = time_path->free_lps;
                    std::move(freelps.begin(), freelps.end(), std::back_inserter(glps));
                    freelps.clear();
                    break;
                }
                case operator_t::F:{
                    //std::cout << "F\n";
                    //print();
                    compile_program();
                    auto& flps = time_path->final_lps;
                    auto& freelps = time_path->free_lps;
                    auto& nextlps = time_path->next_lps;
                   // std::move(freelps.begin(), freelps.end(), std::back_inserter(flps));
                   // std::move(nextlps.begin(), nextlps.end(), std::back_inserter(flps));
                   // freelps.clear();
                   // nextlps.clear();
                    time_path->global_lps.clear();
                    update_next_counter(nextlps, next_ops);
                    update_next_counter(flps, next_ops);
                    advance_timepoint();
                    //std::cout << "-----\n";
                    //print();
                    break;
                }
                case operator_t::X:{
                    compile_program();
                    auto& xlps = time_path->next_lps;
                    auto& freelps = time_path->free_lps;
                    std::move(freelps.begin(), freelps.end(), std::back_inserter(xlps));
                    freelps.clear();
                    time_path->global_lps.clear();
                    update_next_counter(xlps, next_ops);
                    break;
                }
                default:
                    assert(false);
            }
        }
        
        bool AbstractProgramCollection::mergeBuffer::lpsImpossible(const PQL::SimplificationContext& context, temporalContext& tcx, uint32_t solvetime){
            //print();
            if(!time_path){
                return false;
            }
            assert(time_path);
            if(time_path->free_lps.size() != 0){
                assert(time_path->free_lps.size() == 1);
                bool free_impossible = time_path->free_lps[0].prog.isImpossible(context, solvetime);
                if(free_impossible){
                    std::cout << "free impossible\n";
                    return true;
                }
            }

            if(tcx.enable_F_rule){
                for(auto cur = time_path; cur != nullptr; cur = cur->next){
                    for(auto& [lp_tcx, lp] : cur->final_lps){
                        if(lp.isImpossible(context, solvetime)){
                            //std::cout << "F impossible\n";
                            return true;
                        }
                    }
                }
    
                if(context.rules().F_rule){
                    std::cout << "before Final solve\n-----\n";
                    print();
                    std::cout << "-----\n";
                    auto [lps, perms, starts] = setup_solve();
                    if(lps.size() >= 2){
                        bool final_impossible = LinearProgram::solveFinalConjunctionImpossible(lps, perms, starts, context);
                        if(final_impossible){
                            std::cout << "final impossible\n";
                            return true;
                        }
                    }
                }
            }

            if(tcx.enable_X_rule){
                for(auto cur = time_path; cur != nullptr; cur = cur->next){
                    for(auto& [lp_tcx, lp] : cur->next_lps){
                        if(false && !tcx.has_prefix()){
                            double lim = static_cast<double>(lp_tcx.prefix_X);
                            std::cout << "fire limit " << lim << "\n";
                            if(lp.isNStepsImpossible(lim, !context.isDeadlocked(), context, solvetime)){
                                return true;
                            }
                        }else{
                            if(lp.isImpossible(context, solvetime)){
                                return true;
                            }
                        }
                    }
                }

            }

            return false;
        }
        
        // ***********************************
        // AbstractProgramCollection functions
        // ***********************************

        AbstractProgramCollection_ptr AbstractProgramCollection::clone(){
            reset();
            return cloneImpl();
        }

        bool AbstractProgramCollection::satisfiable(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime)
        {   
            reset();
            if (context.timeout() || has_empty || solvetime == 0 || tcx.skip_solve()){ 
                if(context.timeout())
                    std::cout << "returning from timeout\n";
                return true;
            }
            if (_result != UNKNOWN)
            {
                if (_result == IMPOSSIBLE)
                {
                    return _result == POSSIBLE;
                }
            }
            //tcx.update(_operator, _next_ops);
            satisfiableImpl(context, tcx, solvetime);
            assert(_result != UNKNOWN);
            return _result == POSSIBLE;
        }

    
        nextProgram AbstractProgramCollection::get_next_program(){
            return getNextProgramImpl();
        }

        uint32_t AbstractProgramCollection::explorePotency(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            return explorePotencyImpl(context, potencies, maxConfigurationsSolved);
        }

        // *************************
        // UnionCollection functions
        // *************************

        UnionCollection::UnionCollection(std::vector<AbstractProgramCollection_ptr>&& programs)
        : AbstractProgramCollection(), lps(std::move(programs))
        {
            for (auto& p : lps)
                _size += p->size();
        }

        UnionCollection::UnionCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B)
        : AbstractProgramCollection(), lps({A,B})
        {
            has_empty = false;
            for (auto& lp : lps)
            {
                has_empty = has_empty || lp->empty();
                if (lp->known_sat() || has_empty) _result = POSSIBLE;
                if (_result == POSSIBLE) break;
            }
            for (auto& p : lps)
                _size += p->size();
        }

        void UnionCollection::clear()
        {
            lps.clear();
            current = 0;
        }

        void UnionCollection::reset()
        {
            lps[0]->reset();
            current = 0;
        }

        bool UnionCollection::merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run)
        {
            //std::cout << "\t union\n";
            if (current >= lps.size())
            {
                current = 0;
            }

            bool has_more = lps[current]->merge(has_empty, buf, tcx, dry_run);
        
            if(!dry_run)
                buf.apply_operator(_operator, _next_ops);
        
            if (!has_more)
            {
                ++current;
                if (current < lps.size()){
                    lps[current]->reset();
                }
            }

            return current < lps.size();
        }

        AbstractProgramCollection_ptr UnionCollection::cloneImpl(){
            std::vector<AbstractProgramCollection_ptr> members;
            for(int i = 0; i < lps.size(); i++){
                members.push_back(lps[i]->clone());
            }
            auto u = std::make_shared<UnionCollection>(std::move(members));
            u->_next_ops = _next_ops;
            u->_operator = _operator;
            return u;
        }

        void UnionCollection::satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime)
        {
            //std::cout << "solve union\n";
            //tcx.update(_operator, _next_ops);
            for (int i = lps.size() - 1; i >= 0; --i)
            {
                if (lps[i]->satisfiable(context, tcx, solvetime) || context.timeout())
                {
                    _result = POSSIBLE;
                    return;
                }
                else
                {
                    lps.erase(lps.begin() + i);
                }
            }
            if (_result != POSSIBLE){
                //std::cout << "union impossible\n";
                _result = IMPOSSIBLE;
            }

            
        }

        
        nextProgram UnionCollection::getNextProgramImpl(){
            if(lps.size() == 0){
                return {std::make_shared<LinearProgram>(LinearProgram()), false};
            }
            assert(current < lps.size());
            AbstractProgramCollection_ptr prog = lps[current];
            // this is to handle nested unions/merges where the value of np.prog might not be a SingleProgram
            nextProgram np = prog->get_next_program();
            if(!np.hasmore){
                bool hasmore = (current + 1  < lps.size());
                if(hasmore){
                    current++;
                }else{
                    reset();
                }
                return {np.prog, hasmore};
            }else{
                return np;
            }
        } 

        uint32_t UnionCollection::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            for (int i = lps.size() - 1; i >= 0 && maxConfigurationsSolved > 0; --i)
            {
                if (context.potencyTimeout())
                    return 0;

                maxConfigurationsSolved = lps[i]->explorePotency(context, potencies, maxConfigurationsSolved);
                lps.erase(lps.begin() + i); // Continue to the next configuration after erasing the one we just solved
            }

            return maxConfigurationsSolved;
        }

        // constexpr uint16_t MAX_CONFIG = 10;

        // *************************
        // MergeCollection functions
        // *************************

        MergeCollection::MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B)
        : AbstractProgramCollection(), left(A), right(B)
        {
            assert(A);
            assert(B);
            has_empty = left->empty() && right->empty();
            _size = left->size() * right->size();
        }

        void MergeCollection::clear()
        {
            left = nullptr;
            right = nullptr;
        }

        void MergeCollection::reset()
        {
            if (right)
                right->reset();

            merge_right = true;
            more_right  = true;
            rempty = false;

            tmp_prog = LinearProgram();
            mergeBuffer right_buf = mergeBuffer();
            curr = 0;

            _final_programs.clear();

            next_prog = LinearProgram();
        }

        bool MergeCollection::merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run)
        {
            //std::cout << "merge conj\n";
            if (buf.program.knownImpossible()) {
                return false;
            }

            bool lempty = false;
            bool more_left;
           
            //tcx.update(_operator, _next_ops);
    
            while (true)
            {
                lempty = false;
                //LinearProgram prog = program;
                if (merge_right)
                {
                    assert(more_right);
                    rempty = false;
                    //tmp_prog = LinearProgram();
                    //more_right = right->merge(rempty, tmp_prog, tcx, false);
                    right_buf = mergeBuffer();
                    //std::cout << "empty: " <<(right_buf.time_path == nullptr) << "\n";
                    more_right = right->merge(rempty, right_buf, tcx, false);
                    //right_buf.print();
                    left->reset();
                    merge_right = false;
                }
                
                ++curr;
                assert(curr <= _size);

                //more_left = left->merge(lempty, prog, tcx/*, dry_run || curr < nsat*/);
                if(dry_run || curr < nsat){
                    mergeBuffer dry_buf = mergeBuffer();
                    more_left = left->merge(lempty, dry_buf, tcx, true);
                }else{
                    more_left = left->merge(lempty, buf, tcx/*, dry_run || curr < nsat*/);
                    buf.print();
                }

                if (!more_left) merge_right = true;
                if (curr > nsat || !(more_left || more_right))
                {
                    /*if ((!dry_run && prog.knownImpossible()) && (more_left || more_right)) {
                        continue;
                    }

                    if (!dry_run) {
                        program.swap(prog);
                    }*/
                    //if ((!dry_run && buf.program.knownImpossible()) && (more_left || more_right)) {
                    //    continue;
                    //}

                    break;
                }
            }
            //if (!dry_run)
            //    program.make_union(tmp_prog);

            if (!dry_run){
                buf.merge(right_buf);
                buf.apply_operator(_operator, _next_ops);
            }

            has_empty = lempty && rempty;
            return more_left || more_right;
        }

        AbstractProgramCollection_ptr MergeCollection::cloneImpl(){
            auto m = std::make_shared<MergeCollection>(left->clone(), right->clone());
            m->_next_ops = _next_ops;
            m->_operator = _operator;
            return m;
        }

        void MergeCollection::satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime)
        {
            // this is where the magic needs to happen
            //tcx.update(_operator, _next_ops);
            bool hasmore = false;
            do {
                if (context.timeout())
                {
                    _result = POSSIBLE;
                    break;
                }

                //LinearProgram prog;
                
                bool has_empty = false;
                mergeBuffer buf = mergeBuffer(tcx, LinearProgram());

                
                hasmore = merge(has_empty, buf, tcx);
               
                buf.compile_program();
                
                if (has_empty)
                {
                    _result = POSSIBLE;
                    return;
                }
                else
                {
                    //std::cout << "merge before solve:\n\t";
                    //buf.print();
                    if (context.timeout() ||
                        !buf.lpsImpossible(context, tcx, solvetime))
                    {
                        _result = POSSIBLE;
                        break;
                    }
                }
                ++nsat;
            } while (hasmore);
            
            
            if (_result != POSSIBLE)
                _result = IMPOSSIBLE;
        }

        nextProgram MergeCollection::getNextProgramImpl(){
            bool has_empty = false;
            next_prog = LinearProgram();
            temporalContext tcx(false); tcx.update(_operator, _next_ops);
            mergeBuffer buf = mergeBuffer(tcx, next_prog);
            bool hasmore = merge(has_empty, buf, tcx);
            next_prog = buf.program;
            std::shared_ptr<LinearProgram> prog_ptr = std::make_shared<LinearProgram>(next_prog);
            nextProgram np = {prog_ptr, hasmore};

            if(!hasmore){
                reset();
            }

            return np;
        }

        uint32_t MergeCollection::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            bool hasmore = false;
            temporalContext tcx(false); tcx.update(_operator, _next_ops);
            do {
                if (context.potencyTimeout() || maxConfigurationsSolved == 0)
                    return 0;

                LinearProgram prog;
                mergeBuffer buf = mergeBuffer(tcx, prog);
                bool has_empty = false;
                hasmore = merge(has_empty, buf, tcx);
                if (has_empty)
                    return maxConfigurationsSolved;
                else
                {
                    --maxConfigurationsSolved;
                    buf.program.solvePotency(context, potencies);
                }

                ++nsat;
            } while (hasmore);

            return maxConfigurationsSolved;
        }

        // ***********************
        // SingleProgram functions
        // ***********************

        SingleProgram::SingleProgram() : AbstractProgramCollection()
        {
            has_empty = true;
        }

        SingleProgram::SingleProgram(LinearProgram lp) : AbstractProgramCollection()
        {
            program = lp;
            has_empty = ( program.size() == 0 );
        }

        SingleProgram::SingleProgram(LPCache* factory, const Member& lh, int64_t constant, op_t op)
        : AbstractProgramCollection(),
          program(factory->createAndCache(lh.variables()), constant, op, factory)
        {
            has_empty = program.size() == 0;
            assert(!has_empty);
        }

        bool SingleProgram::merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run)
        {   

            //std::cout << "\t\tmerge single\n";
            if (dry_run)
                return false;

            /*tcx.update(_operator, _next_ops);
            switch(tcx._operator){ 
                case operator_t::F:{
                    buf.final_unmerged.push_back({tcx,&this->program});
                    return false;
                }
                case operator_t::X:{
                    buf.next_unmerged.push_back({tcx,&this->program});
                    return false;
                }
                case operator_t::G:{
                    buf.global_conditions.push_back({tcx,&this->program});
                    break;
                }
                default: 
                    break;
            }
            auto& program = buf.program;
            program.make_union(this->program);
            */
            timepoint tp = timepoint();
            //auto program_ptr = std::make_shared<LinearProgram>(LinearProgram(this->program));
            switch(_operator){
                case operator_t::FREE:{
                    tp.free_lps.push_back({tcx, this->program});
                    break;
                }
                case operator_t::G:{
                    tp.global_lps.push_back({tcx, this->program});
                    break;
                }
                case operator_t::F:{
                    tp.final_lps.push_back({tcx, this->program});
                    break;
                }
                case operator_t::X:{
                    tp.next_lps.push_back({tcx, this->program});
                    break;
                }
                default:
                    assert(false);
            }
            buf.time_path = std::make_shared<timepoint>(tp);

            has_empty = this->program.equations().size() == 0;
            assert(has_empty == this->has_empty);
            return false;
        }

        AbstractProgramCollection_ptr SingleProgram::cloneImpl(){
            auto s = std::make_shared<SingleProgram>(LinearProgram(this->program));
            s->_next_ops = _next_ops;
            s->_operator = _operator;
            return s;
        }

        void SingleProgram::satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime)
        {
            //std::cout << "sinsgle program\n";
            // this is where the magic needs to happen
            //tcx.update(_operator, _next_ops);
            if(false && !tcx.has_prefix() && _next_ops >= 1 && _operator >= operator_t::X){
                std::cout << "next ops " << _next_ops << "\n";
                double lim = static_cast<double>(_next_ops);
                std::cout << "fire limit " << lim << "\n";
                if (!program.isNStepsImpossible(lim, (!context.isDeadlocked() && _next_ops == 1), context, solvetime))
                {
                    _result = POSSIBLE;
                }
                else
                {
                    _result = IMPOSSIBLE;
                }
            }else if (!program.isImpossible(context, solvetime))
            {
                _result = POSSIBLE;
            }
            else
            {
                _result = IMPOSSIBLE;
            }
        }

        nextProgram SingleProgram::getNextProgramImpl(){
            return {std::make_shared<LinearProgram>(program), false};
        }

        
        uint32_t SingleProgram::explorePotencyImpl(const PQL::SimplificationContext& context,
            std::vector<uint32_t> &potencies, uint32_t maxConfigurationsSolved)
        {
            if (context.potencyTimeout() || maxConfigurationsSolved == 0)
                return 0;

            program.solvePotency(context, potencies);
            return maxConfigurationsSolved - 1;
        }

    }
}
