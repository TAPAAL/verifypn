#include "PetriEngine/Simplification/LinearPrograms.h"

#include <vector>

namespace PetriEngine {
    namespace Simplification {

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
            tcx.update(_operator, _next_ops);
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
            tcx.update(_operator, _next_ops);
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
           
            tcx.update(_operator, _next_ops);
    
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
            tcx.update(_operator, _next_ops);
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
            //std::cout << "single program\n";
            // this is where the magic needs to happen
            tcx.update(_operator, _next_ops);
            if (!program.isImpossible(context, solvetime))
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
