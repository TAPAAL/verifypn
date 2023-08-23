#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "LinearProgram.h"
#include "../PQL/Contexts.h"
#include "../PetriNet.h"

#include <set>

namespace PetriEngine {
    namespace Simplification {
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

            public:
                virtual ~AbstractProgramCollection() {}
                bool empty() { return has_empty; }

                virtual bool satisfiable(const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max());

                bool known_sat() { return _result == POSSIBLE; }
                bool known_unsat() { return _result == IMPOSSIBLE; }

                virtual void clear() = 0;
                virtual void reset() = 0;
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

            void satisfiableImpl(const PQL::SimplificationContext& context, uint32_t solvetime) override;
            uint32_t explorePotencyImpl(const PQL::SimplificationContext& context,
                                        std::vector<uint32_t> &potencies,
                                        uint32_t maxConfigurationsSolved) override;

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

        public:
            SingleProgram();
            SingleProgram(LPCache* factory, const Member& lh, int64_t constant, op_t op);

            virtual ~SingleProgram() {}

            void clear() override {}
            void reset() override {}
            size_t size() const override { return 1; }
            bool merge(bool& has_empty, LinearProgram& program, bool dry_run = false) override;
        };
    }
}

#endif /* LINEARPROGRAMS_H */
