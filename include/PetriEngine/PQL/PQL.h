/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef PQL_H
#define PQL_H
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>

#include "../PetriNet.h"
#include "../Simplification/LPCache.h"

namespace PetriEngine {
    class ReducingSuccessorGenerator;
    class StubbornSet;
    namespace Simplification
    {
        class Member;
        struct Retval;
    }
    namespace PQL {
        class Visitor;
        class MutatingVisitor;

        using type_id_t = uint8_t;
        constexpr auto untyped = std::numeric_limits<type_id_t>::max();

        template<typename T>
        constexpr type_id_t type_id() {
            if constexpr (std::is_pointer<T>::value) {
                using Q = typename std::remove_pointer<T>::type;
                return type_id<Q>();
            }
            else if constexpr (std::is_const<T>::value) {
                using Q = typename std::remove_cv<T>::type;
                return type_id<Q>();
            }
            else
            {
                return untyped;
            }
        }


        enum CTLType {PATHQEURY = 1, LOPERATOR = 2, EVAL = 3, TYPE_ERROR = -1};
        enum Quantifier { AND = 1, OR = 2, A = 3, E = 4, NEG = 5, COMPCONJ = 6, DEADLOCK = 7, UPPERBOUNDS = 8, PN_BOOLEAN = 9, BControl = 10, EMPTY = -1 };
        enum Path { G = 1, X = 2, F = 3, U = 4, R = 5, PControl = 6, pError = -1 };


        class AnalysisContext;
        class EvaluationContext;
        class DistanceContext;
        class SimplificationContext;

        /** Representation of an expression */
        class Expr {
            int _eval = 0;
        public:
            /** Virtual destructor, an expression should know it subexpressions */
            virtual ~Expr();

            virtual type_id_t type() const = 0;

            [[nodiscard]] virtual bool placeFree() const = 0;

            void setEval(int eval) {
                _eval = eval;
            }

            [[nodiscard]] int getEval() const {
                return _eval;
            }
        };
/******************* NEGATION PUSH STATS  *******************/

        struct negstat_t
        {
            static constexpr std::array _rulename {
                    "EG p-> !EF !p",
                    "AG p-> !AF !p",
                    "!EX p -> AX p",
                    "EX false -> false",
                    "EX true -> !deadlock",
                    "!AX p -> EX p",
                    "AX false -> deadlock",
                    "AX true -> true",
                    "EF !deadlock -> !deadlock",
                    "EF EF p -> EF p",
                    "EF AF p -> AF p",
                    "EF E p U q -> EF q",
                    "EF A p U q -> EF q",
                    "EF .. or .. -> EF .. or EF ..",
                    "AF !deadlock -> !deadlock",
                    "AF AF p -> AF p",
                    "AF EF p -> EF p",
                    "AF .. or EF p -> EF p or AF ..",
                    "AF A p U q -> AF q",
                    "A p U !deadlock -> !deadlock",
                    "A deadlock U q -> q",
                    "A !deadlock U q -> AF q",
                    "A p U AF q -> AF q",
                    "A p U EF q -> EF q",
                    "A p U .. or EF q -> EF q or A p U ..",
                    "E p U !deadlock -> !deadlock",
                    "E deadlock U q -> q",
                    "E !deadlock U q -> EF q",
                    "E p U EF q -> EF q",
                    "E p U .. or EF q -> EF q or E p U ..",
                    "!! p -> p",
                    // LTL rules
                    "F F p -> F p",
                    "F p U q -> F q",
                    "F p or q -> F p or F q",
                    "p U F q -> F q"
            };
            static constexpr size_t nrules = std::tuple_size<decltype(_rulename)>::value;

            negstat_t()
            {
                for(size_t i = 0; i < nrules; ++i) _used[i] = 0;
            }
            void print(std::ostream& stream)
            {
                for(size_t i = 0; i < nrules; ++i) stream << _used[i] << ",";
            }
            void printRules(std::ostream& stream)
            {
                for(size_t i = 0; i < nrules; ++i) stream << _rulename[i] << ",";
            }
            int _used[nrules];
            int& operator[](size_t i) { return _used[i]; }
            bool negated_fireability = false;
        };

        /** Base condition */
        class Condition : public std::enable_shared_from_this<Condition> {
        public:
            enum Result {RUNKNOWN=-1,RFALSE=0,RTRUE=1};
        private:
            bool _inv = false;
            Result _eval = RUNKNOWN;
        public:
            /** Virtual destructor */
            virtual ~Condition();
            /** Get distance to query */
            [[nodiscard]] virtual uint32_t distance(DistanceContext& context) const = 0;

            /** Checks if the condition is trivially true */
            [[nodiscard]] bool isTriviallyTrue();
            /*** Checks if the condition is trivially false */
            [[nodiscard]] bool isTriviallyFalse();

            [[nodiscard]] bool isSatisfied() const
            {
                return _eval == RTRUE;
            }

            void setSatisfied(bool isSatisfied)
            {
                _eval = isSatisfied ? RTRUE : RFALSE;
            }

            void setSatisfied(Result isSatisfied)
            {
                _eval = isSatisfied;
            }

            [[nodiscard]] Result getSatisfied() const
            {
                return _eval;
            }

            void setInvariant(bool isInvariant)
            {
                _inv = isInvariant;
            }

            bool isInvariant()
            {
                return _inv;
            }

            [[nodiscard]] virtual CTLType getQueryType() const = 0;
            [[nodiscard]] virtual Quantifier getQuantifier() const = 0;
            [[nodiscard]] virtual Path getPath() const = 0;
            void toString(std::ostream& os = std::cout);
            virtual type_id_t type() const = 0;
        protected:
            //Value for checking if condition is trivially true or false.
            //0 is undecided (default), 1 is true, 2 is false.
            uint32_t trivial = 0;
        };
        typedef std::shared_ptr<Condition> Condition_ptr;
        using Condition_constptr = std::shared_ptr<const Condition>;
        typedef std::shared_ptr<Expr> Expr_ptr;
    } // PQL
} // PetriEngine

#endif // PQL_H
