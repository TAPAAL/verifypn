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
                        return free_lps.size()  == 0 && final_lps.size()  == 0 && 
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
                        // create a new empty timepoint as the current timepoint, and appends the previous one
                        auto tp = std::make_shared<timepoint>(timepoint());
                
                        tp->next = time_path;
                        time_path = tp;
                        
                    }

                    void compile_program();

                    void apply_operator(operator_t update_op, int next_ops);

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
                        int i = 0;
                        for(auto cur = time_path; cur != nullptr; cur = cur->next){
                            for(int j = 0; j < i; j++)
                                std::cout << "\t";
                            
                            if(i > 0)
                                std::cout << "->";
                            std::cout << "LPS: [" << cur->free_lps.size() << "," << cur->final_lps.size() << "," << cur->next_lps.size() << "," << cur->global_lps.size() << "]\n";
                            i += 1;
                        }
                    }

                    std::tuple<std::vector<LinearProgram*>, std::vector<std::vector<uint32_t>>, std::vector<uint32_t>> setup_solve(){
                        std::vector<LinearProgram*> lps;

                        std::vector<int> fixpoints;

                        for(auto cur = time_path; cur != nullptr; cur = cur->next){
                            if(cur->free_lps.size() > 0){
                                fixpoints.push_back(lps.size());
                                lps.push_back(&cur->free_lps[0].prog);
                            }

                            for(auto& [_tcx, _prog] : cur->final_lps){
                                lps.push_back(&_prog);
                            }

                            for(auto& [_tcx, _prog] : cur->next_lps){
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
                            if(start < lps.size()){
                                std::vector<uint32_t> perm(lps.size() - start);
                                for(int i = 0; i < (lps.size() - start); i++){
                                    perm[i] = start + i;
                                }
                                perms.push_back(perm);
                                starts.push_back(start);
                            }
                        }

                        return std::make_tuple(lps, perms, starts);
                    }
                    
                    bool lpsImpossible(const PQL::SimplificationContext& context, temporalContext& tcx, uint32_t solvetime);
                    
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
                    //std::cout << "update_operator\n";
                    //std::cout << "op: " << update_op << ", " << (update_op == operator_t::X) << "\n";
                    _operator = std::min(_operator, update_op);
                    _next_ops += (update_op == operator_t::X);
                    //std::cout << "next ops: " << _next_ops << "\n";
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
