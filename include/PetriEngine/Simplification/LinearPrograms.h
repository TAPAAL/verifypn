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
            protected:
                enum result_t { UNKNOWN, IMPOSSIBLE, POSSIBLE };
                result_t _result = result_t::UNKNOWN;
                bool has_empty = false;

                virtual void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) = 0;
                virtual uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                                    std::vector<uint32_t> &potencies,
                                                    uint32_t maxConfigurationsSolved) = 0;
                virtual nextProgram getNextProgramImpl() = 0;

            public:
                /* do not create two iterators to the same collection, as they change the internal state */
                struct NextProgramIterator{
                    using interator_category = std::input_iterator_tag;
                    using difference_type = std::ptrdiff_t;
                    using value_type = LinearProgram*;
                    NextProgramIterator() = default;

                    NextProgramIterator(AbstractProgramCollection* collection)
                        : _collection(collection) {
                            _collection->reset();
                            _next = _collection->get_next_program();
                            //if(!_next.hasmore)
                            //    _collection = nullptr;
                            _next_ptr = _next.prog.get();}
                    const value_type& operator*() const {
                        return _next_ptr;
                    }

                    const LinearProgram* operator->() const {
                        return _next.prog.get();
                    }

                    NextProgramIterator& operator++() {
                        if(!_next.hasmore){ 
                            _collection = nullptr;
                        }else{
                            _next = _collection->get_next_program();
                            _next_ptr = _next.prog.get();
                        }
                        return *this;
                    }

                    NextProgramIterator operator++(int) {
                        auto tmp = *this;
                        ++*this;
                        return tmp;
                    }

                    friend bool operator==(const NextProgramIterator& a,
                                        const NextProgramIterator& b)
                    {
                        return a._collection == b._collection;
                    }

                    friend bool operator!=(const NextProgramIterator& a,
                                        const NextProgramIterator& b)
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

                        NextProgramIterator begin() {
                            return NextProgramIterator(_collection);
                        }

                        NextProgramIterator end() {
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

                virtual bool satisfiable(const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max());
                virtual nextProgram  get_next_program();

                bool known_sat() { return _result == POSSIBLE; }
                bool known_unsat() { return _result == IMPOSSIBLE; }

                virtual void clear() = 0;
                virtual void reset() = 0;
                //virtual AbstractProgramCollection clone() = 0;
                virtual size_t size() const = 0;
                virtual bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) = 0;


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

            void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) override;
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
            bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) override;
        };

        class MergeCollection : public AbstractProgramCollection
        {
        protected:
            AbstractProgramCollection_ptr left = nullptr;
            AbstractProgramCollection_ptr right = nullptr;

            LinearProgram tmp_prog;
            bool merge_right = true;
            bool more_right  = true;
            bool rempty = false;
            size_t nsat = 0;
            size_t curr = 0;
            size_t _size = 0;

            LinearProgram next_prog;

            void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) override;
            uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                        std::vector<uint32_t> &potencies,
                                        uint32_t maxConfigurationsSolved) override;
            
            nextProgram getNextProgramImpl() override;

        public:
            MergeCollection(const AbstractProgramCollection_ptr& A, const AbstractProgramCollection_ptr& B);

            void clear() override;
            void reset() override;
            size_t size() const override { return _size - nsat; }
            bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) override;
        };

        class SingleProgram : public AbstractProgramCollection {
        private:
            LinearProgram program;

        protected:
            void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) override;
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
            bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) override;

            LinearProgram getProgram() const{
                return program;
            }


        };
    }
}

#endif /* LINEARPROGRAMS_H */
