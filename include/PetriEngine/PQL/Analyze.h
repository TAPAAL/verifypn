/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>,
 *                     Rasmus Tollund <rtollu18@student.aau.dk>
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
#ifndef VERIFYPN_ANALYZE_H
#define VERIFYPN_ANALYZE_H

#include "PetriEngine/PQL/MutatingVisitor.h"

namespace PetriEngine { namespace PQL {

    void analyze(Condition* condition, AnalysisContext& context);
    void analyze(Condition_ptr condition, AnalysisContext& context);

    class AnalyzeVisitor : public MutatingVisitor {
    public:
        explicit AnalyzeVisitor(AnalysisContext& context)
            : _context(context) {}

    private:
        AnalysisContext& _context;

        void _accept(NaryExpr *element) override;

        void _accept(CommutativeExpr *element) override;

        void _accept(MinusExpr *element) override;

        void _accept(LiteralExpr *element) override;

        void _accept(IdentifierExpr *element) override;

        void _accept(UnfoldedIdentifierExpr *element) override;

        void _accept(FireableCondition *element) override;

        void _accept(CompareConjunction *element) override;

        void _accept(CompareCondition *element) override;

        void _accept(NotCondition *element) override;

        void _accept(BooleanCondition *element) override;

        void _accept(DeadlockCondition *element) override;

        void _accept(KSafeCondition *element) override;

        void _accept(LogicalCondition *element) override;

        void _accept(QuasiLivenessCondition *element) override;

        void _accept(LivenessCondition *element) override;

        void _accept(UnfoldedFireableCondition *element) override;

        void _accept(StableMarkingCondition *element) override;

        void _accept(UpperBoundsCondition *element) override;

        void _accept(UnfoldedUpperBoundsCondition *element) override;

        void _accept(UntilCondition *element) override;

        void _accept(SimpleQuantifierCondition *element) override;
    };
} }

#endif //VERIFYPN_ANALYZE_H
