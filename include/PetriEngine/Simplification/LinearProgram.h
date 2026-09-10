#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H

#include "../PetriNet.h"
#include "Member.h"
#include "Vector.h"
#include "../PQL/Contexts.h"

#include <algorithm>
#include <unordered_set>
#include <memory>
#include <glpk.h>


namespace PetriEngine {
    namespace Simplification {
        using REAL = double;

        struct equation_t
        {
            double upper = std::numeric_limits<double>::infinity();
            double lower = -std::numeric_limits<double>::infinity();
            double operator [] (size_t i) const
            {
                return i > 0 ? upper : lower;
            }
            Vector* row;
            
            bool operator <(const equation_t& other) const
            {
                return row < other.row;
            }
        };

        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::vector<equation_t> _equations;

            bool _addEquationsImpl(glp_prob* lp, const PQL::SimplificationContext& context, int& rowno, std::vector<REAL>& row, std::vector<int32_t>& indir, const std::vector<equation_t>& equations, int32_t variable_shift = 0) const;
            bool addEquations(glp_prob* lp, const PQL::SimplificationContext& context, int& rowno, std::vector<REAL>& row, std::vector<int32_t>& indir, const std::vector<equation_t>& equations) const;
            bool addEquationsShifted(glp_prob* lp, const PQL::SimplificationContext& context, int& rowno, std::vector<REAL>& row, std::vector<int32_t>& indir, const std::vector<equation_t>& equations, int32_t variable_shift = 0) const;

            result_t solve_built_lp(glp_prob* lp, const PQL::SimplificationContext& context, uint32_t solvetime , bool delete_lp = true) const;
            result_t solve_and_set(glp_prob* lp, const PQL::SimplificationContext& context, uint32_t solvetime, bool delete_lp = true);
        public:
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(_equations, other._equations);
            }

            LinearProgram() {}
            virtual ~LinearProgram();

            LinearProgram(const LinearProgram& other)
            : _result(other._result), _equations(other._equations) {}

            LinearProgram(Vector* vec, int64_t constant, op_t op, LPCache* factory);
            size_t size() const
            {
                return _equations.size();
            }

            const std::vector<equation_t>& equations() const
            {
                return _equations;
            }

            bool knownImpossible() const { return _result == result_t::IMPOSSIBLE; }
            bool knownPossible() const { return _result == result_t::POSSIBLE; }


            bool isImpossible(const PQL::SimplificationContext& context, uint32_t solvetime);
            bool isFinalImpossibleWith(const LinearProgram* withLp, bool is_next, bool is_strict, const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max()) const;
            bool isFinalImpossibleWithN(const std::vector<uint32_t>& permutation, const std::vector<LinearProgram*>& lps, bool is_next, bool is_strict, const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max()) const;
            bool isNStepsImpossible(double firelimit, bool strict, const PQL::SimplificationContext& context, uint32_t solvetime = std::numeric_limits<uint32_t>::max());
            void solvePotency(const PQL::SimplificationContext& context, std::vector<uint32_t>& potencies);

            void make_union(const LinearProgram& other);

            std::ostream& print(std::ostream& ss, size_t indent = 0) const
            {
                for (size_t i = 0; i < indent ; ++i) ss << "\t";
                ss << "### LP\n";

                for (const equation_t& eq : _equations)
                {
                    for (size_t i = 0; i < indent ; ++i) ss << "\t";
                    eq.row->print(ss);
                    ss << " IN [" << eq.lower << ", " << eq.upper << "]\n";
                }

                for (size_t i = 0; i < indent ; ++i) ss << "\t";
                ss << "### LP DONE";
                return ss;
            }
            static std::vector<std::pair<double,bool>> bounds(const PQL::SimplificationContext& context, uint32_t solvetime, const std::vector<uint32_t>& places);
        };
    }
}

#endif /* LINEARPROGRAM_H */
