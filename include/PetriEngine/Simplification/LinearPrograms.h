#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PQL/Contexts.h"
#include "../PetriNet.h"

#include <set>

namespace PetriEngine {
    namespace Simplification {
        class AbstractProgramCollection;

        struct nextProgram
        {
            std::shared_ptr<LinearProgram> prog;
            bool hasmore;
        };

        

        
        class AbstractProgramCollection
        {   
            public:
                enum operator_t {F, X, G, FREE};
                
                struct temporalContext{
                    operator_t _operator = operator_t::FREE;
                    int _next_ops = 0;
                    bool _is_invariant = false;
                    int prefix_F = 0;
                    int prefix_X = 0;
                    int prefix_G = 0;

                    bool allow_basic_solve = true;
                    bool enable_F_rule = true;
                    bool enable_X_rule = true;

                    temporalContext(bool is_invariant) : _is_invariant(is_invariant) {};
                    temporalContext() : temporalContext(false){};

                    void update(operator_t new_operator, int leading_next_depth){
                        _operator = std::min(_operator, new_operator);
                        _next_ops += leading_next_depth;
                    }

                    std::tuple<int, int, int> push_prefix(operator_t update_op, bool negated){
                        auto pre_state = std::make_tuple(prefix_F, prefix_G, prefix_X);

                        prefix_X += (update_op == operator_t::X);
                        prefix_F += ( update_op == operator_t::F && !negated ) ||
                                    ( update_op == operator_t::G &&  negated );
                                    
                        prefix_G += ( update_op == operator_t::F &&  negated ) ||
                                    ( update_op == operator_t::G && !negated );
                        return pre_state;
                    }

                    std::tuple<int, int, int> clear_context(){
                        auto pre_state = std::make_tuple(prefix_F, prefix_G, prefix_X);

                        prefix_X = 0;
                        prefix_F = 0;
                        prefix_G = 0;

                        return pre_state;
                    }

                    void set_prefix(std::tuple<int,int,int>& state){
                        prefix_F = std::get<0>(state);
                        prefix_G = std::get<1>(state);
                        prefix_X = std::get<2>(state);
                    }

                    bool has_prefix() const{
                        return (prefix_F + prefix_G + prefix_X) > 0;
                    }

                    bool skip_solve() const{
                        return !allow_basic_solve;
                    }
                };
                struct temporalProgram{
                    temporalContext tcx;
                    LinearProgram prog;
                };
                struct timepoint{
                    std::shared_ptr<timepoint> next = nullptr;
                    std::vector<temporalProgram> free_lps = {};
                    std::vector<temporalProgram> final_lps = {};
                    std::vector<temporalProgram> next_lps = {};
                    std::vector<temporalProgram> global_lps = {};

                    LinearProgram program;
                    bool is_compiled = false;

                    timepoint() = default;

                    bool is_empty() const{
                        return program.size()  == 0 && final_lps.size()  == 0 && 
                                next_lps.size() == 0 && global_lps.size() == 0;
                    }
                };

            protected:
                
                struct mergeBuffer{
                    temporalContext pre_ctx;
                    temporalContext post_ctx;
                    LinearProgram program;
                    std::shared_ptr<timepoint> time_path = nullptr;
                    mergeBuffer() = default;
                    mergeBuffer(temporalContext _ctx, LinearProgram _prog) : pre_ctx(_ctx), program(_prog){
                        post_ctx = temporalContext(true);
                    };

                    void advance_timepoint(){
                        assert(time_path);
                        if(time_path->is_empty())
                            return;
                        assert(time_path->is_compiled);
                        // create a new empty timepoint as the current timepoint, and append the previous one
                        auto tp = std::make_shared<timepoint>(timepoint());
                        std::swap(tp, time_path);
                        time_path->next = tp;
                        
                    }

                    void compile_program(){
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

                            for(auto& [tcx, lp] : time_path->final_lps)
                                lp.make_union(global_program);
                            
                            for(auto& [tcx, lp] : time_path->next_lps)
                                lp.make_union(global_program);
                            
                            time_path->global_lps.clear();
                            //time_path->global_lps.push_back({temporalContext(), std::make_shared<LinearProgram>(global_program)});
                        }
                        
                        time_path->free_lps.clear();
                        time_path->free_lps.push_back({temporalContext(), free_program}); 
                    }

                    void apply_operator(operator_t update_op, int next_ops){
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
                                compile_program();
                                auto& flps = time_path->final_lps;
                                auto& freelps = time_path->free_lps;
                                auto& nextlps = time_path->next_lps;
                                std::move(freelps.begin(), freelps.end(), std::back_inserter(flps));
                                std::move(nextlps.begin(), nextlps.end(), std::back_inserter(flps));
                                freelps.clear();
                                nextlps.clear();
                                time_path->global_lps.clear();
                                update_next_counter(flps, next_ops);
                                advance_timepoint();
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

                    void merge(mergeBuffer& other){
                        std::copy(other.time_path->free_lps.begin(), other.time_path->free_lps.end(), std::back_inserter(time_path->free_lps));
                        std::copy(other.time_path->final_lps.begin(), other.time_path->final_lps.end(), std::back_inserter(time_path->final_lps));
                        std::copy(other.time_path->global_lps.begin(), other.time_path->global_lps.end(), std::back_inserter(time_path->global_lps));
                        std::copy(other.time_path->next_lps.begin(), other.time_path->next_lps.end(), std::back_inserter(time_path->next_lps));
                    }

                    void print(){
                        if(!time_path){
                            std::cout << "LPS: [null]";
                            return;
                        }
                        std::cout << "LPS: [" << time_path->free_lps.size() << "," << time_path->final_lps.size() << "," << time_path->next_lps.size() << "," << time_path->global_lps.size() << "]\n";
                    }

                    std::tuple<std::vector<LinearProgram*>, std::vector<std::vector<uint32_t>>, std::vector<uint32_t>> setup_solve(){
                        std::vector<LinearProgram*> lps;

                        std::vector<int> fixpoints;

                        for(auto cur = time_path; cur != nullptr; cur = cur->next){
                            if(cur->free_lps.size() > 0){
                                fixpoints.push_back(lps.size());
                                lps.push_back(&cur->free_lps[0].prog);
                            }

                            for(auto [_tcx, _prog] : cur->final_lps){
                                lps.push_back(&_prog);
                            }

                            for(auto [_tcx, _prog] : cur->next_lps){
                                lps.push_back(&_prog);
                            }
                        }

                        std::vector<std::vector<uint32_t>> perms;
                        std::vector<uint32_t> starts;
                        if(fixpoints.empty()){
                            std::vector<uint32_t> perm(lps.size());
                            for(int i = 0; i < lps.size(); i++){
                                perm[i] = i;
                            }
                            perms.push_back(perm);
                            starts.push_back(0);
                        }
                        else{
                            int start = 0;
                            for(int fix : fixpoints){
                                if(fix == start){
                                    start = fix + 1;
                                    continue;
                                }
                                std::vector<uint32_t> perm(fix - start);
                                for(int i = 0; i < (fix - start); i++){
                                    perm[i] = start + i;
                                }
                                perms.push_back(perm);
                                starts.push_back(start);
                                start = fix + 1;
                            }
                        }

                        return std::make_tuple(lps, perms, starts);
                    }
                    
                    bool lpsImpossible(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime){
                        //print();
                        if(!time_path){
                            return false;
                        }
                        assert(time_path);
                        if(time_path->free_lps.size() != 0){
                            assert(time_path->free_lps.size() == 1);
                            bool free_impossible = time_path->free_lps[0].prog.isImpossible(context, solvetime);
                            if(free_impossible){
                                //std::cout << "free impossible\n";
                                return true;
                            }
                        }

                        if(tcx.enable_F_rule){
                            for(auto cur = time_path; cur != nullptr; cur = cur->next){
                                for(auto& [tcx, lp] : cur->final_lps){
                                    if(lp.isImpossible(context, solvetime)){
                                        //std::cout << "F impossible\n";
                                        return true;
                                    }
                                }
                            }
                
                            if(context.rules().F_rule){
                                auto [lps, perms, starts] = setup_solve();
                                if(lps.size() >= 2){
                                    bool final_impossible = LinearProgram::solveFinalConjunctionImpossible(lps, perms, starts, context);
                                    if(final_impossible)
                                        return true;
                                }
                            }
                        }

                        if(tcx.enable_X_rule){
                            for(auto cur = time_path; cur != nullptr; cur = cur->next){
                                for(auto& [tcx, lp] : cur->next_lps){
                                    if(lp.isImpossible(context, solvetime)){
                                        return true;
                                    }
                                }
                            }

                        }

                        return false;
                        

                    }
                    
                    private:
                    void update_next_counter(std::vector<temporalProgram>& lps, int next_ops){
                        if(next_ops == 0)
                            return;
                        for(size_t i = 0; i < lps.size(); i++){
                            lps[i].tcx._next_ops += next_ops;
                        }
                    }
                };

                enum result_t { UNKNOWN, IMPOSSIBLE, POSSIBLE };
                result_t _result = result_t::UNKNOWN;
                operator_t _operator = operator_t::FREE;
                int _next_ops = 0;
                bool has_empty = false;

                virtual std::shared_ptr<AbstractProgramCollection> cloneImpl() = 0;
                virtual void satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime) = 0;
                virtual uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                                    std::vector<uint32_t> &potencies,
                                                    uint32_t maxConfigurationsSolved) = 0;
                virtual nextProgram getNextProgramImpl() = 0;

            public:
                /* it is not safe to have two iterators to the same collection, as they change the internal state */
                struct ProgramIterator{
                    using interator_category = std::input_iterator_tag;
                    using difference_type = std::ptrdiff_t;
                    using value_type = LinearProgram*;
                    ProgramIterator() = default;

                    ProgramIterator(AbstractProgramCollection* collection)
                        : _collection(collection) {
                            _collection->reset();
                            _next = _collection->get_next_program();
                            _next_ptr = _next.prog.get();
                        }
                    
                    const value_type& operator*() const {
                        return _next_ptr;
                    }

                    const LinearProgram* operator->() const {
                        return _next.prog.get();
                    }

                    ProgramIterator& operator++() {
                        if(!_next.hasmore){ 
                            _collection = nullptr;
                        }else{
                            _next = _collection->get_next_program();
                            _next_ptr = _next.prog.get();
                        }
                        return *this;
                    }

                    ProgramIterator operator++(int) {
                        auto tmp = *this;
                        ++*this;
                        return tmp;
                    }

                    friend bool operator==(const ProgramIterator& a,
                                        const ProgramIterator& b)
                    {
                        return a._collection == b._collection;
                    }

                    friend bool operator!=(const ProgramIterator& a,
                                        const ProgramIterator& b)
                    {
                        return !(a == b);
                    }
                    private:
                        AbstractProgramCollection* _collection = nullptr;
                        LinearProgram* _next_ptr;
                        nextProgram _next;
                };
                        
                class ProgRange {
                    public:
                        ProgRange(AbstractProgramCollection* collection)
                            : _collection(collection){}

                        ProgramIterator begin() {
                            return ProgramIterator(_collection);
                        }

                        ProgramIterator end() {
                            return {};
                        }

                    private:
                        AbstractProgramCollection* _collection;
                    };

                ProgRange AllProgs() {
                    reset();
                    return ProgRange(this);
                }
                
                virtual ~AbstractProgramCollection() {}
                bool empty() { return has_empty; }

                void update_operator(operator_t update_op){
                    if(update_op != _operator || update_op == operator_t::X){
                        _result = result_t::UNKNOWN;
                    }
                    _operator = std::min(_operator, update_op);
                    _next_ops += update_op == operator_t::X;
                }

                virtual std::shared_ptr<AbstractProgramCollection> clone();
                virtual bool satisfiable(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime = std::numeric_limits<uint32_t>::max());
                virtual nextProgram  get_next_program();

                bool known_sat() { return _result == POSSIBLE; }
                bool known_unsat() { return _result == IMPOSSIBLE; }

                virtual void clear() = 0;
                virtual void reset() = 0;
                //virtual AbstractProgramCollection clone() = 0;
                virtual size_t size() const = 0;
                virtual bool merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run = false) = 0;


                virtual uint32_t explorePotency(const PQL::SimplificationContext& context,
                                                std::vector<uint32_t> &potencies,
                                                uint32_t maxConfigurationsSolved = std::numeric_limits<uint32_t>::max());
        };

        typedef std::shared_ptr<AbstractProgramCollection> AbstractProgramCollection_ptr;

        class UnionCollection : public AbstractProgramCollection
        {
        protected:
            std::vector<AbstractProgramCollection_ptr> lps;
            size_t current = 0;
            size_t _size = 0;

            std::shared_ptr<AbstractProgramCollection> cloneImpl() override;
            void satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime) override;
            uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                        std::vector<uint32_t> &potencies,
                                        uint32_t maxConfigurationsSolved) override;
            
            nextProgram getNextProgramImpl() override;

        public:
            UnionCollection(std::vector<AbstractProgramCollection_ptr>&& programs);
            UnionCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B);

            void clear() override;
            void reset() override;
            size_t size() const override { return _size; }
            bool merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run = false) override;
        };

        class MergeCollection : public AbstractProgramCollection
        {
        protected:
            AbstractProgramCollection_ptr left = nullptr;
            AbstractProgramCollection_ptr right = nullptr;

            LinearProgram tmp_prog; 
            mergeBuffer right_buf;
            bool merge_right = true;
            bool more_right  = true;
            bool rempty = false;
            size_t nsat = 0;
            size_t curr = 0;
            size_t _size = 0;

            LinearProgram next_prog;
            std::vector<LinearProgram*> _final_programs;

            std::shared_ptr<AbstractProgramCollection> cloneImpl() override;
            void satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime) override;
            uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                        std::vector<uint32_t> &potencies,
                                        uint32_t maxConfigurationsSolved) override;
            
            nextProgram getNextProgramImpl() override;

        public:
            MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B);

            void clear() override;
            void reset() override;
            size_t size() const override { return _size - nsat; }
            bool merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run = false) override;
        };

        class SingleProgram : public AbstractProgramCollection {
        private:
            LinearProgram program;

        protected:
            std::shared_ptr<AbstractProgramCollection> cloneImpl() override;
            void satisfiableImpl(const PQL::SimplificationContext& context, temporalContext tcx, uint32_t solvetime) override;
            uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                        std::vector<uint32_t> &potencies,
                                        uint32_t maxConfigurationsSolved) override;
            nextProgram getNextProgramImpl() override;
        public:
            SingleProgram();
            SingleProgram(LinearProgram lp);
            SingleProgram(LPCache* factory, const Member& lh, int64_t constant, op_t op);


            virtual ~SingleProgram() {}

            void clear() override {}
            void reset() override {}
            size_t size() const override { return 1; }
            bool merge(bool& has_empty, mergeBuffer& buf, temporalContext tcx, bool dry_run = false) override;

            LinearProgram getProgram() const{
                return program;
            }


        };
    }
}

#endif /* LINEARPROGRAMS_H */
