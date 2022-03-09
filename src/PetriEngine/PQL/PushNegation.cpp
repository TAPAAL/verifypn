/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#include "utils/errors.h"
#include "PetriEngine/PQL/PushNegation.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Evaluation.h"
#include "PetriEngine/PQL/CTLVisitor.h"


// Macro to ensure that returns are done correctly
#ifndef NDEBUG
#define RETURN(x) { assert(x); return_value = x; has_returned = true; return;}
#else
#define RETURN(x) {return_value = x; return;}
#endif

namespace PetriEngine { namespace PQL {

    Condition_ptr initialMarkingRW(const std::function<Condition_ptr()>& func, negstat_t& stats, const EvaluationContext& context, bool _nested, bool _negated, bool initrw)
    {
        auto res = func();
        if(!_nested && initrw)
        {
            auto e = PetriEngine::PQL::evaluate(res.get(), context);
            if(e != Condition::RUNKNOWN)
            {
                if(res->getQuantifier() == E && res->getPath() == F)
                {
                    auto e = static_cast<ECondition*>(res.get());
                    auto f = static_cast<FCondition*>((*e)[0].get());
                    if((*f)[0]->type() == type_id<UnfoldedUpperBoundsCondition>())
                    {
                        return res;
                    }
                }
                return BooleanCondition::getShared(e);
            }
        }
        return res;
    }

    Condition_ptr
    pushNegation(Condition_ptr cond, negstat_t &stats, const EvaluationContext &context, bool nested, bool negated, bool initrw) {
        PushNegationVisitor pn_visitor(stats, context, nested, negated, initrw);
        Visitor::visit(pn_visitor, cond);
        return pn_visitor.return_value;
    }

    bool CompareCondition::isTrivial() const
    {
        auto remdup = [](auto& a, auto& b){
            auto ai = a->_ids.begin();
            auto bi = b->_ids.begin();
            while(ai != a->_ids.end() && bi != b->_ids.end())
            {
                while(ai != a->_ids.end() && ai->first < bi->first) ++ai;
                if(ai == a->_ids.end()) break;
                if(ai->first == bi->first)
                {
                    ai = a->_ids.erase(ai);
                    bi = b->_ids.erase(bi);
                }
                else
                {
                    ++bi;
                }
                if(bi == b->_ids.end() || ai == a->_ids.end()) break;
            }
        };
        if(auto p1 = std::dynamic_pointer_cast<PlusExpr>(_expr1))
            if(auto p2 = std::dynamic_pointer_cast<PlusExpr>(_expr2))
                remdup(p1, p2);

        if(auto m1 = std::dynamic_pointer_cast<MultiplyExpr>(_expr1))
            if(auto m2 = std::dynamic_pointer_cast<MultiplyExpr>(_expr2))
                remdup(m1, m2);

        if(auto p1 = std::dynamic_pointer_cast<CommutativeExpr>(_expr1))
            if(auto p2 = std::dynamic_pointer_cast<CommutativeExpr>(_expr2))
                return p1->_exprs.size() + p1->_ids.size() + p2->_exprs.size() + p2->_ids.size() == 0;
        return _expr1->placeFree() && _expr2->placeFree();
    }

    uint32_t
    CompareCondition::_distance(DistanceContext &c, std::function<uint32_t(uint32_t, uint32_t, bool)>&& d) const {
        return d(evaluate(_expr1.get(), c), evaluate(_expr2.get(), c), c.negated());
    }

    void PushNegationVisitor::_accept(ControlCondition *element) {
        auto res = subvisit((*element)[0], false, negated);
        return_value = std::make_shared<ControlCondition>(res);
    }

    Condition_ptr PushNegationVisitor::pushneg_EG(GCondition *element) {
        ++stats[0];
        auto af_cond = std::make_shared<ACondition>(std::make_shared<FCondition>(std::make_shared<NotCondition>(element->getCond())));
        return subvisit(af_cond, nested, !negated);
    }

    Condition_ptr PushNegationVisitor::pushneg_AG(GCondition *element) {
        ++stats[1];
        auto ef_cond = std::make_shared<ECondition>(std::make_shared<FCondition>(std::make_shared<NotCondition>(element->getCond())));
        return subvisit(ef_cond, nested, !negated);
    }

    Condition_ptr PushNegationVisitor::pushneg_EX(XCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto a = subvisit(element->getCond(), true, negated);
            if (negated) {
                ++stats[2];
                return subvisit(std::make_shared<ACondition>(std::make_shared<XCondition>(a)), nested, false);
            } else {
                if (a == BooleanCondition::FALSE_CONSTANT) {
                    ++stats[3];
                    return a;
                }
                if (a == BooleanCondition::TRUE_CONSTANT) {
                    ++stats[4];
                    return std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK);
                }
                a = std::make_shared<ECondition>(std::make_shared<XCondition>(a));
            }
            return a;
        }, stats, context, nested, negated, initrw);
        return cond;
    }

    Condition_ptr PushNegationVisitor::pushneg_AX(XCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto a = subvisit(element->getCond(), true, negated);
            if (negated) {
                ++stats[5];
                return subvisit(std::make_shared<ECondition>(std::make_shared<XCondition>(a)), nested, false);
            } else {
                if (a == BooleanCondition::TRUE_CONSTANT) {
                    ++stats[6];
                    return a;
                }
                if (a == BooleanCondition::FALSE_CONSTANT) {
                    ++stats[7];
                    return DeadlockCondition::DEADLOCK;
                }
                a = std::make_shared<ACondition>(std::make_shared<XCondition>(a));
            }
            return a;
        }, stats, context, nested, negated, initrw);
        return cond;
    }


    Condition_ptr PushNegationVisitor::pushneg_EF(FCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto a = subvisit(element->getCond(), true, false);

            if (a->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(a.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[8];
                    return subvisit(a, nested, negated);
                }
            }

            if (!isTemporal(a)) {
                auto res = std::make_shared<ECondition>(std::make_shared<FCondition>(a));
                if (negated) return std::make_shared<NotCondition>(res);
                return res;
            }

            if (a->type() == type_id<ECondition>()) {
                auto econd = static_cast<ECondition*>(a.get());

                if (econd->getCond()->type() == type_id<FCondition>()) {
                    ++stats[9];
                    if (negated) a = std::make_shared<NotCondition>(a);
                    return a;
                } else if (econd->getCond()->type() == type_id<UntilCondition>()) {
                    ++stats[11];
                    auto ucond = static_cast<UntilCondition*>(econd->getCond().get());
                    a = subvisit(std::make_shared<ECondition>(std::make_shared<FCondition>((*ucond)[1])), nested, negated);
                    return a;
                }
            } else if (a->type() == type_id<ACondition>()) {
                auto acond = static_cast<ACondition*>(a.get());

                if (acond->getCond()->type() == type_id<FCondition>()) {
                    ++stats[10];
                    auto fcond = static_cast<FCondition*>(acond->getCond().get());
                    a = subvisit(std::make_shared<ECondition>(std::make_shared<FCondition>((*fcond)[0])), nested, negated);
                    return a;
                } else if (acond->getCond()->type() == type_id<UntilCondition>()) {
                    ++stats[12];
                    auto ucond = static_cast<UntilCondition*>(acond->getCond().get());
                    a = subvisit(std::make_shared<ECondition>(std::make_shared<FCondition>((*ucond)[1])), nested, negated);
                    return a;
                }
            } else if (a->type() == type_id<OrCondition>()) {
                auto orcond = static_cast<OrCondition*>(a.get());
                if (!isTemporal(orcond)) {
                    Condition_ptr b = std::make_shared<ECondition>(std::make_shared<FCondition>(a));
                    if (negated) b = std::make_shared<NotCondition>(b);
                    return b;
                }
                ++stats[13];
                std::vector<Condition_ptr> pef, atomic;
                for (auto &i: *orcond) {
                    pef.push_back(std::make_shared<ECondition>(std::make_shared<FCondition>(i)));
                }
                a = subvisit(makeOr(pef), nested, negated);
                return a;
            }

            Condition_ptr b = std::make_shared<ECondition>(std::make_shared<FCondition>(a));
            if (negated) b = std::make_shared<NotCondition>(b);
            return b;
        }, stats, context, nested, negated, initrw);
        return cond;
    }


    Condition_ptr PushNegationVisitor::pushneg_AF(FCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto a = subvisit(element->getCond(), true, false);

            if (a->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(a.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[14];
                    return subvisit(a, nested, negated);
                }
            }

            if (a->type() == type_id<ACondition>()) {
                auto acond = static_cast<ACondition*>(a.get());

                if (acond->getCond()->type() == type_id<FCondition>()) {
                    ++stats[15];
                    if (negated) return std::make_shared<NotCondition>(a);
                    return a;
                } else if (acond->getCond()->type() == type_id<UntilCondition>()) {
                    ++stats[18];
                    auto ucond = static_cast<UntilCondition*>(acond->getCond().get());
                    return subvisit(std::make_shared<ACondition>(std::make_shared<FCondition>((*ucond)[1])), nested, negated);
                }
            }

            if (a->type() == type_id<ECondition>()) {
                auto econd = static_cast<ECondition*>(a.get());
                if (econd->getCond()->type() == type_id<FCondition>()) {
                    ++stats[16];
                    if (negated) return std::make_shared<NotCondition>(a);
                    return a;
                }
            }

            if (a->type() == type_id<OrCondition>()) {
                auto cond = static_cast<OrCondition*>(a.get());

                std::vector<Condition_ptr> pef, npef;
                for (auto &i: *cond) {
                    if (i->type() == type_id<ECondition>() && static_cast<ECondition*>(i.get())->getCond()->type() == type_id<FCondition>()) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[17] += pef.size();
                    pef.push_back(std::make_shared<ACondition>(std::make_shared<FCondition>(makeOr(npef))));
                    return subvisit(makeOr(pef), nested, negated);
                }
            }

            auto b = std::make_shared<ACondition>(std::make_shared<FCondition>(a));
            if (negated) return std::make_shared<NotCondition>(b);
            return b;
        }, stats, context, nested, negated, initrw);
        return cond;
    }

    Condition_ptr PushNegationVisitor::pushneg_AU(UntilCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            // We push negation and convert this to a ReleaseCondition if it is negated
            auto b = subvisit(element->getCond2(), true, negated);
            auto a = subvisit(element->getCond1(), true, negated);

            if (b->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(b.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[19];
                    return subvisit(b, nested, negated);
                }
            } else if (a == DeadlockCondition::DEADLOCK) {
                ++stats[20];
                return subvisit(b, nested, negated);
            } else if (a->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(a.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[21];
                    return subvisit(std::make_shared<ACondition>(std::make_shared<FCondition>(b)), nested, negated);
                }
            }

            if (b->type() == type_id<ACondition>()) {
                auto acond = static_cast<ACondition*>(b.get());
                if (acond->getCond()->type() == type_id<FCondition>()) {
                    ++stats[22];
                    return subvisit(acond, nested, negated);
                }
            } else if (b->type() == type_id<ECondition>()) {
                auto econd = static_cast<ECondition*>(b.get());
                if (econd->getCond()->type() == type_id<FCondition>()) {
                    ++stats[23];
                    if (negated) return std::make_shared<NotCondition>(b);
                    return b;
                }
            } else if (b->type() == type_id<OrCondition>()) {
                auto orcond = static_cast<OrCondition*>(b.get());
                std::vector<Condition_ptr> pef, npef;
                for (auto &i: *orcond) {
                    if (i->type() == type_id<ECondition>() && static_cast<ECondition*>(i.get())->getCond()->type() == type_id<FCondition>()) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[24] += pef.size();
                    if (npef.size() != 0) {
                        pef.push_back(std::make_shared<ACondition>(std::make_shared<UntilCondition>(element->getCond1(), makeOr(npef))));
                    } else {
                        ++stats[23];
                        --stats[24];
                    }
                    return subvisit(makeOr(pef), nested, negated);
                }
            }

            if (negated)
                return std::make_shared<ECondition>(std::make_shared<ReleaseCondition>(a, b));
            else 
                return std::make_shared<ACondition>(std::make_shared<UntilCondition>(a, b));
        }, stats, context, nested, negated, initrw);
        return cond;
    }


    Condition_ptr PushNegationVisitor::pushneg_EU(UntilCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto b = subvisit(element->getCond2(), true, false);
            auto a = subvisit(element->getCond1(), true, false);

            if (b->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(b.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[25];
                    return subvisit(b, nested, negated);
                }
            } else if (a == DeadlockCondition::DEADLOCK) {
                ++stats[26];
                return subvisit(b, nested, negated);
            } else if (a->type() == type_id<NotCondition>()) {
                auto notcond = static_cast<NotCondition*>(a.get());
                if ((*notcond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[27];
                    return subvisit(std::make_shared<ECondition>(std::make_shared<FCondition>(b)), nested, negated);
                }
            }

            if (b->type() == type_id<ECondition>() && static_cast<ECondition*>(b.get())->getCond()->type() == type_id<FCondition>()) {
                ++stats[28];
                if (negated) return std::make_shared<NotCondition>(b);
                return b;
            } else if (auto cond = dynamic_cast<OrCondition *>(b.get())) {
                std::vector<Condition_ptr> pef, npef;
                for (auto &i: *cond) {
                    if (i->type() == type_id<ECondition>() && static_cast<ECondition*>(i.get())->getCond()->type() == type_id<FCondition>()) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[29] += pef.size();
                    if (npef.size() != 0) {
                        pef.push_back(std::make_shared<ECondition>(std::make_shared<UntilCondition>(element->getCond1(), makeOr(npef))));
                        ++stats[28];
                        --stats[29];
                    }
                    return subvisit(makeOr(pef), nested, negated);
                }
            }
            if (negated)
                return std::make_shared<ACondition>(std::make_shared<ReleaseCondition>(a, b));
            else
                return std::make_shared<ECondition>(std::make_shared<UntilCondition>(a, b));
        }, stats, context, nested, negated, initrw);
        return cond;
    }

/*LTL negation push*/
    void PushNegationVisitor::_accept(UntilCondition* element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            // Push negation and convert this to a Release if it is negated
            auto b = subvisit(element->getCond2(), true, negated);
            auto a = subvisit(element->getCond1(), true, negated);

            if (auto cond = std::dynamic_pointer_cast<FCondition>(b)) {
                static_assert(negstat_t::nrules >= 35);
                ++stats[34];
                return b;
            }

            if (negated)
                return std::make_shared<ReleaseCondition>(a, b);
            else
                return std::make_shared<UntilCondition>(a, b);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(ReleaseCondition* element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            // Push negation and convert this to a UntilCondition if it is negated
            auto b = subvisit(element->getCond2(), true, negated);
            auto a = subvisit(element->getCond1(), true, negated);

            if (auto cond = std::dynamic_pointer_cast<FCondition>(b)) {
                static_assert(negstat_t::nrules >= 35);
                ++stats[34];
                return b;
            }

            if (negated)
                return std::make_shared<UntilCondition>(a, b);
            else
                return std::make_shared<ReleaseCondition>(a, b);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(XCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto res = subvisit(element->getCond(), true, negated);
            if (res == BooleanCondition::TRUE_CONSTANT || res == BooleanCondition::FALSE_CONSTANT) {
                return res;
            }
            return std::make_shared<XCondition>(res);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(FCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            auto a = subvisit(element->getCond(), true, false);
            if (!isTemporal(a)) {
                auto res = std::make_shared<FCondition>(a);
                if (negated) return std::make_shared<NotCondition>(res);
                return res;
            }

            if (dynamic_cast<FCondition *>(a.get())) {
                ++stats[31];
                if (negated) a = std::make_shared<NotCondition>(a);
                return a;
            } else if (auto cond = dynamic_cast<UntilCondition *>(a.get())) {
                ++stats[32];
                return subvisit(std::make_shared<FCondition>((*cond)[1]), nested, negated);
            } else if (auto cond = dynamic_cast<OrCondition *>(a.get())) {
                if (!isTemporal(cond)) {
                    Condition_ptr b = std::make_shared<FCondition>(a);
                    if (negated) b = std::make_shared<NotCondition>(b);
                    return b;
                }
                ++stats[33];
                std::vector<Condition_ptr> distributed;
                for (auto &i: *cond) {
                    distributed.push_back(std::make_shared<FCondition>(i));
                }
                return subvisit(makeOr(distributed), nested, negated);
            } else {
                Condition_ptr b = std::make_shared<FCondition>(a);
                if (negated) b = std::make_shared<NotCondition>(b);
                return b;
            }
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(ACondition *element) {
        if ((*element)[0]->type() == type_id<FCondition>()) {
            RETURN(pushneg_AF(static_cast<FCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<GCondition>()) {
            RETURN(pushneg_AG(static_cast<GCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<XCondition>()) {
            RETURN(pushneg_AX(static_cast<XCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<UntilCondition>()) {
            RETURN(pushneg_AU(static_cast<UntilCondition*>((*element)[0].get())));
        } else {
            auto e_cond = ECondition(std::make_shared<NotCondition>(element->getCond()));
            RETURN(subvisit(&e_cond, nested, !negated))
        }
    }


    void PushNegationVisitor::_accept(ECondition *element) {
        if ((*element)[0]->type() == type_id<FCondition>()) {
            RETURN(pushneg_EF(static_cast<FCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<GCondition>()) {
            RETURN(pushneg_EG(static_cast<GCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<XCondition>()) {
            RETURN(pushneg_EX(static_cast<XCondition*>((*element)[0].get())));
        } else if ((*element)[0]->type() == type_id<UntilCondition>()) {
            RETURN(pushneg_EU(static_cast<UntilCondition*>((*element)[0].get())));
        } else {
            // we forward the negated flag, we flip the outer quantifier later!
            auto _sub = subvisit(element->getCond(), nested, negated);
            if (negated) RETURN(std::make_shared<ACondition>(_sub))
            else RETURN(std::make_shared<ECondition>(_sub))
        }
    }

    void PushNegationVisitor::_accept(GCondition *element) {
        auto f_cond = FCondition(std::make_shared<NotCondition>(element->getCond()));
        RETURN(subvisit(&f_cond, nested, !negated))
    }

/*Boolean connectives */
    Condition_ptr PushNegationVisitor::pushAnd(const std::vector<Condition_ptr> &_conds, bool _nested, bool negate_children) {
        std::vector<Condition_ptr> nef, other;
        for (auto &c: _conds) {
            auto n = subvisit(c, _nested, negate_children);
            if (n->isTriviallyFalse()) return n;
            if (n->isTriviallyTrue()) continue;
            if (n->type() == type_id<NotCondition>()) {
                auto neg = static_cast<NotCondition*>(n.get());
                if (neg->getCond()->type() == type_id<ECondition>() && static_cast<ECondition*>(neg->getCond().get())->getCond()->type() == type_id<FCondition>()) {
                    auto econd = static_cast<ECondition*>(neg->getCond().get());
                    auto fcond = static_cast<FCondition*>(econd->getCond().get());
                    nef.push_back(fcond->getCond());
                } else {
                    other.emplace_back(n);
                }
            } else {
                other.emplace_back(n);
            }
        }
        if (nef.size() + other.size() == 0)
            return BooleanCondition::TRUE_CONSTANT;
        if (nef.size() + other.size() == 1) {
            return nef.size() == 0 ?
                   other[0] :
                   std::make_shared<NotCondition>(std::make_shared<ECondition>(std::make_shared<FCondition>(nef[0])));
        }
        if (nef.size() != 0)
            other.push_back(
                    std::make_shared<NotCondition>(
                            std::make_shared<ECondition>(
                                    std::make_shared<FCondition>(
                                        makeOr(nef)))));
        if (other.size() == 1) return other[0];
        auto res = makeAnd(other);
        return res;
    }

    Condition_ptr PushNegationVisitor::pushOr(const std::vector<Condition_ptr> &_conds, bool _nested, bool negate_children) {
        std::vector<Condition_ptr> nef, other;
        for (auto &c: _conds) {
            auto n = subvisit(c, _nested, negate_children);
            if (n->isTriviallyTrue()) {
                return n;
            }
            if (n->isTriviallyFalse()) continue;
            if (n->type() == type_id<ECondition>() && static_cast<ECondition*>(n.get())->getCond()->type() == type_id<FCondition>()) {
                auto econd = static_cast<ECondition*>(n.get());
                auto fcond = static_cast<FCondition*>(econd->getCond().get());
                nef.push_back((*fcond)[0]);
            } else {
                other.emplace_back(n);
            }
        }
        if (nef.size() + other.size() == 0)
            return BooleanCondition::FALSE_CONSTANT;
        if (nef.size() + other.size() == 1) {
            return nef.size() == 0 ? other[0] : std::make_shared<ECondition>(std::make_shared<FCondition>(nef[0]));
        }
        if (nef.size() != 0)
            other.push_back(
                    std::make_shared<ECondition>(
                            std::make_shared<FCondition>(
                                makeOr(nef))));
        if (other.size() == 1) return other[0];
        return makeOr(other);
    }

    void PushNegationVisitor::_accept(OrCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            return negated ? pushAnd(element->getOperands(), nested, true) :
                   pushOr(element->getOperands(), nested, false);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }


    void PushNegationVisitor::_accept(AndCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            return negated ? pushOr(element->getOperands(), nested, true) :
                   pushAnd(element->getOperands(), nested, false);

        }, stats, context, nested, negated, initrw);
        RETURN(cond);
    }

    void PushNegationVisitor::_accept(CompareConjunction *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            return std::make_shared<CompareConjunction>(*element, negated);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }


    void PushNegationVisitor::_accept(NotCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            if (negated) ++stats[30];
            return subvisit(element->getCond(), nested, !negated);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    template<typename T> Condition_ptr
    PushNegationVisitor::pushFireableNegation(const std::string &name, const Condition_ptr &compiled) {
        if (compiled)
            return subvisit(compiled, nested, negated);
        if (negated) {
            stats.negated_fireability = true;
            return std::make_shared<NotCondition>(std::make_shared<T>(name));
        } else
            return std::make_shared<T>(name);
    }

    void PushNegationVisitor::_accept(UnfoldedFireableCondition *element) {
        RETURN(pushFireableNegation<UnfoldedFireableCondition>(element->getName(), element->getCompiled()))
    }

    void PushNegationVisitor::_accept(FireableCondition *element) {
        RETURN(pushFireableNegation<FireableCondition>(element->getName(), element->getCompiled()))
    }

    void PushNegationVisitor::_accept(LessThanCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            if (element->isTrivial())
                return BooleanCondition::getShared(evaluate(element, context) xor negated);
            if (negated) return std::make_shared<LessThanOrEqualCondition>(element->getExpr2(), element->getExpr1());
            else return std::make_shared<LessThanCondition>(element->getExpr1(), element->getExpr2());
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(LessThanOrEqualCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            if (element->isTrivial())
                return BooleanCondition::getShared(evaluate(element, context) xor negated);
            if (negated) return std::make_shared<LessThanCondition>(element->getExpr2(), element->getExpr1());
            else return std::make_shared<LessThanOrEqualCondition>(element->getExpr1(), element->getExpr2());
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    Condition_ptr PushNegationVisitor::pushEqual(CompareCondition *org, bool _negated, bool noteq) {
        if (org->isTrivial())
            return BooleanCondition::getShared(evaluate(org, context) xor _negated);
        for (auto i: {0, 1}) {
            if ((*org)[i]->placeFree() && evaluate((*org)[i].get(), context) == 0) {
                if (_negated == noteq)
                    return std::make_shared<LessThanOrEqualCondition>((*org)[(i + 1) % 2],
                                                                      std::make_shared<LiteralExpr>(0));
                else
                    return std::make_shared<LessThanOrEqualCondition>(std::make_shared<LiteralExpr>(1),
                                                                      (*org)[(i + 1) % 2]);
            }
        }
        if (_negated == noteq) return std::make_shared<EqualCondition>((*org)[0], (*org)[1]);
        else return std::make_shared<NotEqualCondition>((*org)[0], (*org)[1]);
    }

    void PushNegationVisitor::_accept(NotEqualCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            return pushEqual(element, negated, true);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }


    void PushNegationVisitor::_accept(EqualCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            return pushEqual(element, negated, false);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(BooleanCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            if (negated) return BooleanCondition::getShared(!element->value);
            else return BooleanCondition::getShared(element->value);
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(DeadlockCondition *element) {
        auto cond = initialMarkingRW([&]() -> Condition_ptr {
            if (negated) return std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK);
            else return DeadlockCondition::DEADLOCK;
        }, stats, context, nested, negated, initrw);
        RETURN(cond)
    }

    void PushNegationVisitor::_accept(UpperBoundsCondition* element) {
        if (negated) {
            throw base_error("UPPER BOUNDS CANNOT BE NEGATED!");
        }
        if(element->getCompiled())
            Visitor::visit(this, element->getCompiled());
        else
            RETURN(element->clone())
    }

    void PushNegationVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        if (negated) {
            throw base_error("UPPER BOUNDS CANNOT BE NEGATED!");
        }
        RETURN(std::make_shared<UnfoldedUpperBoundsCondition>(element->places(), element->getMax(), element->getOffset()));
    }

    void PushNegationVisitor::_accept(ShallowCondition *element) {
        if(element->getCompiled()) {
            RETURN(subvisit(element->getCompiled(), nested, negated))
        } else {
            if(negated) {
                RETURN(std::static_pointer_cast<Condition>(std::make_shared<NotCondition>(element->clone())))
            } else {
                RETURN(element->clone());
            }
        }
    }

    void PushNegationVisitor::_accept(KSafeCondition* element) {
        _accept(static_cast<ShallowCondition*>(element));
    }

    void PushNegationVisitor::_accept(LivenessCondition* element) {
        _accept(static_cast<ShallowCondition*>(element));
    }

    void PushNegationVisitor::_accept(QuasiLivenessCondition* element) {
        _accept(static_cast<ShallowCondition*>(element));
    }

    void PushNegationVisitor::_accept(StableMarkingCondition* element) {
        _accept(static_cast<ShallowCondition*>(element));
    }

    Condition_ptr PushNegationVisitor::subvisit(Condition* condition, bool _nested, bool _negated) {
    {
            bool old_nested = nested;
            bool old_negated = negated;
            nested = _nested;
            negated = _negated;

//            if (condition->type() == type_id<ECondition>())
//                if ((*static_cast<ECondition*>(condition))[0]->type() == type_id<NotCondition>())
//                    std::cout << "E!" << std::endl;

//            condition->toString(std::cout); std::cout << " IN GO " << std::endl;
            Visitor::visit(this, condition);

//        if (return_value->type() == type_id<NotCondition>())
//                if ((*static_cast<ECondition*>(condition))[0]->type() == type_id<NotCondition>())
//            std::cout << "HERE" << std::endl;

//            PetriEngine::PQL::IsCTLVisitor isCtlVisitor3;
//            Visitor::visit(isCtlVisitor3, return_value);
//            std::cout << isCtlVisitor3.isCTL << std::endl;
//            return_value->toString(std::cout); std::cout << std::endl;
#ifndef NDEBUG
            assert(has_returned); // Subvisit should return value
            has_returned = false;
#endif

            nested = old_nested;
            negated = old_negated;

            return return_value;
        }
    }
} }