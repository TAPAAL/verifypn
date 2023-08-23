/* Copyright (C) 2023  Malo Dautry
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

#include "PetriEngine/PQL/PotencyVisitor.h"

#define RETURN(x) { _return_value = x; return; }

namespace PetriEngine { namespace PQL {

    void initPotencyVisit(const std::shared_ptr<Condition> element,
                               SimplificationContext& context,
                               std::vector<uint32_t> &potencies,
                               uint32_t maxConfigurationsSolved)
    {
        PotencyVisitor potency_initializer(context);
        Visitor::visit(potency_initializer, element);

        // Explore the configurations and solve the LPs, then update the potencies
        potency_initializer.get_return_value().lps->explorePotency(context, potencies, maxConfigurationsSolved);
    }

    RetvalPot PotencyVisitor::simplify_or(const LogicalCondition *element)
    {
        std::vector<AbstractProgramCollection_ptr> lps;
        for (const auto &c: element->getOperands())
        {
            Visitor::visit(this, c);
            auto r = std::move(_return_value);
            assert(r.lps);
            lps.push_back(r.lps);
        }
        // Make a UnionCollection containing all the LPs from the OR Condition
        return RetvalPot(std::make_shared<UnionCollection>(std::move(lps)));
    }

    RetvalPot PotencyVisitor::simplify_and(const LogicalCondition *element)
    {
        std::vector<AbstractProgramCollection_ptr> lpsv;
        for (auto &c: element->getOperands())
        {
            Visitor::visit(this, c);
            auto r = std::move(_return_value);
            lpsv.emplace_back(r.lps);
        }
        // Make MergeCollections combining two by two the LPs from the AND Condition
        auto lps = mergeLps(std::move(lpsv));
        return RetvalPot(std::move(lps));
    }

    /************ Auxiliary functions for quantifier simplification ***********/

    RetvalPot PotencyVisitor::simplify_AG(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    RetvalPot PotencyVisitor::simplify_AF(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    RetvalPot PotencyVisitor::simplify_AX(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    RetvalPot PotencyVisitor::simplify_EG(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    RetvalPot PotencyVisitor::simplify_EF(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    RetvalPot PotencyVisitor::simplify_EX(RetvalPot &r)
    {
        return std::move(_return_value);
    }

    template<typename Quantifier>
    RetvalPot PotencyVisitor::simplify_simple_quantifier(RetvalPot &r)
    {
        static_assert(std::is_base_of_v<SimpleQuantifierCondition, Quantifier>);
        return std::move(_return_value);
    }

    /******* PotencyVisitor accepts ********/

    void PotencyVisitor::_accept(const NotCondition *element)
    {
        _context.negate();
        Visitor::visit(this, element->getCond());
        _context.negate();
        // No return, since it will already be set by visit call
    }

    void PotencyVisitor::_accept(const AndCondition *element)
    {
        if (_context.potencyTimeout())
        {
            if (_context.negated())
            {
                RETURN(RetvalPot(std::make_shared<NotCondition>(makeAnd(element->getOperands()))))
            }
            else
            {
                RETURN(RetvalPot(makeAnd(element->getOperands())))
            }
        }
        if (_context.negated())
        {
            RETURN(simplify_or(element))
        }
        else
        {
            RETURN(simplify_and(element))
        }
    }

    void PotencyVisitor::_accept(const OrCondition *element)
    {
        if (_context.potencyTimeout())
        {
            if (_context.negated())
            {
                RETURN(RetvalPot(std::make_shared<NotCondition>(makeOr(element->getOperands()))))
            }
            else
            {
                RETURN(RetvalPot(makeOr(element->getOperands())))
            }
        }
        if (_context.negated())
        {
            RETURN(simplify_and(element))
        }
        else
        {
            RETURN(simplify_or(element))
        }
    }

    void PotencyVisitor::_accept(const LessThanCondition *element)
    {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);

        AbstractProgramCollection_ptr lps;
        if (!_context.potencyTimeout() && m1.canAnalyze() && m2.canAnalyze())
        {
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                  (_context.negated() ? Simplification::OP_GE
                                                                      : Simplification::OP_LT));
        }
        else
        {
            lps = std::make_shared<SingleProgram>();
        }

        RETURN(RetvalPot(std::move(lps)))
    }

    void PotencyVisitor::_accept(const LessThanOrEqualCondition *element)
    {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);

        AbstractProgramCollection_ptr lps;
        if (!_context.potencyTimeout() && m1.canAnalyze() && m2.canAnalyze())
        {
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                  (_context.negated() ? Simplification::OP_GT
                                                                      : Simplification::OP_LE));
        }
        else
        {
            lps = std::make_shared<SingleProgram>();
        }

        assert(lps);

        RETURN(RetvalPot(std::move(lps)))
    }

    void PotencyVisitor::_accept(const EqualCondition *element)
    {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);
        std::shared_ptr<AbstractProgramCollection> lps;
        if (!_context.potencyTimeout() && m1.canAnalyze() && m2.canAnalyze())
        {
            if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2))
            {
                // RETURN(RetvalPot(BooleanCondition::getShared(
                //     _context.negated() ? (m1.constant() != m2.constant()) : (m1.constant() == m2.constant()))))
                return;
            }
            else
            {
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                if (_context.negated())
                {
                    m2 = m1;
                    lps = std::make_shared<UnionCollection>(
                            std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                            Simplification::OP_GT),
                            std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                            Simplification::OP_LT));
                }
                else
                {
                    lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant, Simplification::OP_EQ);
                }
            }
        }
        else
        {
            lps = std::make_shared<SingleProgram>();
        }

        RETURN(RetvalPot(std::move(lps)))
    }

    void PotencyVisitor::_accept(const NotEqualCondition *element)
    {
        Member m1 = constraint((*element)[0].get(), _context);
        Member m2 = constraint((*element)[1].get(), _context);
        std::shared_ptr<AbstractProgramCollection> lps;
        if (!_context.potencyTimeout() && m1.canAnalyze() && m2.canAnalyze())
        {
            if ((m1.isZero() && m2.isZero()) || m1.substrationIsZero(m2))
            {
                // RETURN(RetvalPot(std::make_shared<BooleanCondition>(
                //     _context.negated() ? (m1.constant() == m2.constant()) : (m1.constant() != m2.constant()))))
                return;
            }
            else
            {
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                if (!_context.negated())
                {
                    m2 = m1;
                    lps = std::make_shared<UnionCollection>(
                            std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                            Simplification::OP_GT),
                            std::make_shared<SingleProgram>(_context.cache(), std::move(m2), constant,
                                                            Simplification::OP_LT));
                }
                else
                {
                    lps = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                          Simplification::OP_EQ);
                }
            }
        }
        else
        {
            lps = std::make_shared<SingleProgram>();
        }

        RETURN(RetvalPot(std::move(lps)))
    }

    void PotencyVisitor::_accept(const DeadlockCondition *element)
    {
        return;
    }

    void PotencyVisitor::_accept(const CompareConjunction *element)
    {
        if (_context.potencyTimeout())
        {
            // RETURN(RetvalPot(std::make_shared<CompareConjunction>(*element, _context.negated())))
            return;
        }
        std::vector<AbstractProgramCollection_ptr> lpsv;
        auto neg = _context.negated() != element->isNegated();
        std::vector<CompareConjunction::cons_t> nconstraints;
        for (auto &c: element->constraints())
        {
            nconstraints.push_back(c);
            if (c._lower != 0)
            {
                auto m2 = memberForPlace(c._place, _context);
                Member m1(c._lower);
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                std::shared_ptr<SingleProgram> lp;
                if (!neg)
                {
                    lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                         Simplification::OP_LE);
                }
                else
                {
                    lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                         Simplification::OP_GT);
                }
                lpsv.push_back(lp);
            }

            if (c._upper != std::numeric_limits<uint32_t>::max())
            {
                auto m1 = memberForPlace(c._place, _context);
                Member m2(c._upper);
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                std::shared_ptr<SingleProgram> lp;
                if (!neg)
                {
                    lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                              Simplification::OP_LE);
                }
                else
                {
                    lp = std::make_shared<SingleProgram>(_context.cache(), std::move(m1), constant,
                                                              Simplification::OP_GT);
                }
                lpsv.push_back(lp);
            }

            assert(nconstraints.size() > 0);
            if (nconstraints.back()._lower == 0 && nconstraints.back()._upper == std::numeric_limits<uint32_t>::max())
                nconstraints.pop_back();

            if (neg)
                assert(nconstraints.size() <= lpsv.size() * 2);
        }

        if (!neg)
        {
            auto lps = mergeLps(std::move(lpsv));

            if (lps == nullptr && !_context.potencyTimeout())
            {
                // Should I return; instead of overwriting the lps (and giving it en empty value)?
                RETURN(RetvalPot(BooleanCondition::getShared(!neg)))
            }

            RETURN(RetvalPot(std::move(lps)))
        }
        else
        {
            if (nconstraints.size() == 0)
            {
                // Idem should I return?
                RETURN(RetvalPot(BooleanCondition::getShared(!neg)))
            }

            RETURN(RetvalPot(std::make_shared<UnionCollection>(std::move(lpsv))))
        }
    }

    void PotencyVisitor::_accept(const UnfoldedUpperBoundsCondition *element)
    {
        std::vector<UnfoldedUpperBoundsCondition::place_t> next;
        std::vector<uint32_t> places;
        for (auto &p: element->places())
            places.push_back(p._place);
        const auto nplaces = element->places().size();
        const auto bounds = LinearProgram::bounds(_context, _context.getLpTimeout(), places);
        double offset = element->getOffset();
        for (size_t i = 0; i < nplaces; ++i)
        {
            if (bounds[i].first != 0 && !bounds[i].second)
                next.emplace_back(element->places()[i], bounds[i].first);
            if (bounds[i].second)
                offset += bounds[i].first;
        }
        if (bounds[nplaces].second)
        {
            next.clear();
            RETURN(RetvalPot(std::make_shared<UnfoldedUpperBoundsCondition>
                                 (next, 0, bounds[nplaces].first + element->getOffset())))
        }
        RETURN(RetvalPot(std::make_shared<UnfoldedUpperBoundsCondition>
                             (next, bounds[nplaces].first - offset, offset)))
    }

    void PotencyVisitor::_accept(const ControlCondition *condition)
    {
        // TODO: what to do here? What is that condition?
        Visitor::visit(this, condition->getCond());
        RETURN(RetvalPot())
    }

    void PotencyVisitor::_accept(const EFCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AG(_return_value) : simplify_EF(_return_value))
    }

    void PotencyVisitor::_accept(const EXCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AX(_return_value) : simplify_EX(_return_value))
    }

    void PotencyVisitor::_accept(const AXCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EX(_return_value) : simplify_AX(_return_value))
    }

    void PotencyVisitor::_accept(const AFCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EG(_return_value) : simplify_AF(_return_value))
    }

    void PotencyVisitor::_accept(const EGCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_AF(_return_value) : simplify_EG(_return_value))
    }

    void PotencyVisitor::_accept(const AGCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_EF(_return_value) : simplify_AG(_return_value))
    }

    void PotencyVisitor::_accept(const EUCondition *condition)
    {
        // cannot push negation any further
        bool neg = _context.negated();
        _context.setNegate(false);

        Visitor::visit(this, (*condition)[1]);
        Visitor::visit(this, (*condition)[0]);

        _context.setNegate(neg);

        // if (_context.negated())
        // {
        //     RETURN(RetvalPot(std::make_shared<NotCondition>(
        //                 std::make_shared<EUCondition>(r1.formula, r2.formula))))
        // }
        // else
        // {
        //     RETURN(RetvalPot(std::make_shared<EUCondition>(r1.formula, r2.formula)))
        // }
        return;
    }

    void PotencyVisitor::_accept(const AUCondition *condition)
    {
        // cannot push negation any further
        bool neg = _context.negated();
        _context.setNegate(false);

        Visitor::visit(this, condition->getCond2());
        Visitor::visit(this, condition->getCond1());

        _context.setNegate(neg);

        // if (_context.negated())
        // {
        //     RETURN(RetvalPot(std::make_shared<NotCondition>(
        //             std::make_shared<AUCondition>(r1.formula, r2.formula))))
        // }
        // else
        // {
        //     RETURN(RetvalPot(std::make_shared<AUCondition>(r1.formula, r2.formula)))
        // }
        return;
    }

    void PotencyVisitor::_accept(const UntilCondition *condition)
    {
        bool neg = _context.negated();
        _context.setNegate(false);

        Visitor::visit(this, condition->getCond2());
        Visitor::visit(this, condition->getCond1());

        _context.setNegate(neg);

        // if (_context.negated())
        // {
        //     RETURN(RetvalPot(std::make_shared<NotCondition>(
        //             std::make_shared<UntilCondition>(r1.formula, r2.formula))))
        // }
        // else
        // {
        //     RETURN(RetvalPot(std::make_shared<UntilCondition>(r1.formula, r2.formula)))
        // }
        return;
    }

    void PotencyVisitor::_accept(const FCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_simple_quantifier<GCondition>(_return_value)
                                  : simplify_simple_quantifier<FCondition>(_return_value))
    }

    void PotencyVisitor::_accept(const GCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(_context.negated() ? simplify_simple_quantifier<FCondition>(_return_value)
                                  : simplify_simple_quantifier<GCondition>(_return_value))
    }

    void PotencyVisitor::_accept(const XCondition *condition)
    {
        Visitor::visit(this, condition->getCond());
        RETURN(simplify_simple_quantifier<XCondition>(_return_value))
    }

    void PotencyVisitor::_accept(const BooleanCondition *condition)
    {
        // TODO: what should I do here?
        if (_context.negated())
        {
            RETURN(RetvalPot(BooleanCondition::getShared(!condition->value)))
        }
        else
        {
            RETURN(RetvalPot(BooleanCondition::getShared(condition->value)))
        }
    }

    void PotencyVisitor::_accept(const PathSelectCondition* condition)
    {
        // TODO: what should I do here?
        if (condition->offset() != 0)
        {
            Visitor::visit(this, condition->child());
            RETURN(RetvalPot())
        }
        else
        {
            auto res = std::make_shared<PathSelectCondition>(condition->name(), condition->child(), condition->offset());
            if (_context.negated())
                RETURN(RetvalPot(std::make_shared<NotCondition>(res)))
            else
                RETURN(RetvalPot(res))
        }
    }
} }
