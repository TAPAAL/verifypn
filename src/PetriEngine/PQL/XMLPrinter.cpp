/* Copyright (C) 2011  Peter Gjøl Jensen <root@petergjoel.dk>,
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
#include "PetriEngine/PQL/XMLPrinter.h"
namespace PetriEngine {
    namespace PQL {

        void XMLPrinter::print(const Condition& c, const std::string& name)
        {
            XMLPrinter::Tag p(this, "property");
            outputLine("<id>", name, "</id>");
            outputLine("<description>Simplified</description>");
            {
                XMLPrinter::Tag f(this, "formula");
                c.visit(*this);
            }
        }

        std::ostream& XMLPrinter::generateTabs() {
            for(uint32_t i = 0; i < tabs*tab_size; ++i) {
                os << ' ';
            }
            return os;
        }

        void XMLPrinter::openXmlTag(const char* tag) {
            generateTabs() << "<" << tag << ">";
            newline();
            ++tabs;
        }

        void XMLPrinter::closeXmlTag(const char* tag) {
            --tabs;
            generateTabs() << "</" << tag << ">";
            newline();
        }

        std::ostream& XMLPrinter::newline() {
            if(print_newlines)
                os << '\n';
            return os;
        }

        void XMLPrinter::_accept(const NotCondition *element) {
            Tag t(this, "negation");
            element->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const AndCondition *element) {
            if(element->empty())
            {
                BooleanCondition::TRUE_CONSTANT->visit(*this);
                return;
            }
            if(element->size() == 1)
            {
                (*element)[0]->visit(*this);
                return;
            }
            {
                Tag t(this, "conjunction");
                for(auto& e : *element)
                    e->visit(*this);
            }
        }

        void XMLPrinter::_accept(const OrCondition *element) {
            if(element->empty())
            {
                BooleanCondition::FALSE_CONSTANT->visit(*this);
                return;
            }
            if(element->size() == 1)
            {
                (*element)[0]->visit(*this);
                return;
            }
            {
                Tag d(this, "disjunction");
                for(auto& e : *element)
                    e->visit(*this);
            }
        }

        void XMLPrinter::_accept(const LessThanCondition *element) {
            Tag t(this, "integer-lt");
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void XMLPrinter::_accept(const LessThanOrEqualCondition *element) {
            Tag t(this, "integer-le");
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void XMLPrinter::_accept(const EqualCondition *element) {
            Tag t(this, "integer-eq");
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void XMLPrinter::_accept(const NotEqualCondition *element) {
            Tag t(this, "integer-ne");
            (*element)[0]->visit(*this);
            (*element)[1]->visit(*this);
        }

        void XMLPrinter::_accept(const DeadlockCondition *element) {
            outputLine("<deadlock/>");
        }

        void XMLPrinter::_accept(const CompareConjunction *element) {
            if(element->isNegated()) {
                generateTabs() << "<negation>";
            }

            if(element->constraints().empty()) BooleanCondition::TRUE_CONSTANT->visit(*this);
            else
            {
                bool single = element->constraints().size() == 1 &&
                              (element->constraints()[0]._lower == 0 ||
                               element->constraints()[0]._upper == std::numeric_limits<uint32_t>::max());
                if(!single)
                    openXmlTag("conjunction");
                for(auto& c : element->constraints())
                {
                    if(c._lower != 0)
                    {
                        openXmlTag("integer-ge");
                        openXmlTag("tokens-count");
                        outputLine("<place>", c._name, "</place>");
                        closeXmlTag("tokens-count");
                        outputLine("<integer-constant>", c._lower, "</integer-constant>");
                        closeXmlTag("integer-ge");
                    }
                    if(c._upper != std::numeric_limits<uint32_t>::max())
                    {
                        openXmlTag("integer-le");
                        openXmlTag("tokens-count");
                        outputLine("<place>", c._name, "</place>");
                        closeXmlTag("tokens-count");
                        outputLine("<integer-constant>", c._upper, "</integer-constant>");
                        closeXmlTag("integer-le");
                    }
                }
                if(!single)
                    closeXmlTag("conjunction");
            }
            if(element->isNegated()) {
                generateTabs() << "</negation>";
            }
        }

        void XMLPrinter::_accept(const UnfoldedUpperBoundsCondition *element) {
            Tag t(this, "place-bound");
            for(auto& p : element->places()) {
                outputLine("<place>", p._name, "</place>");
            }
        }

        void XMLPrinter::_accept(const ControlCondition *condition) {
            Tag t(this, "control");
            (*condition)[0]->visit(*this);
        }

        void XMLPrinter::_accept(const ACondition *condition) {
            Tag a(this, "all-paths");
            condition->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const ECondition *condition) {
            Tag e(this, "exists-path");
            condition->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const GCondition *condition) {
            Tag g(this, "globally");
            condition->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const FCondition *condition) {
            Tag f(this, "finally");
            condition->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const XCondition *condition) {
            Tag n(this, "next");
            condition->getCond()->visit(*this);
        }

        void XMLPrinter::_accept(const UntilCondition *condition) {
            Tag u(this, "until");
            {
                {
                    Tag b(this, "before");
                    (*condition)[0]->visit(*this);
                }
                {
                    Tag r(this, "reach");
                    (*condition)[1]->visit(*this);
                }
            }
        }

        void XMLPrinter::_accept(const UnfoldedFireableCondition *element) {
            outputLine("<is-fireable><transition>", element->getName(), "</transition></is-fireable>");
        }

        void XMLPrinter::_accept(const BooleanCondition *element) {
            outputLine(element->value ? "<true/>": "<false/>");
        }

        void XMLPrinter::_accept(const UnfoldedIdentifierExpr *element) {
            if (token_count) {
                outputLine("<place>", element->name(), "</place>");
            }
            else
            {
                Tag tc(this, "tokens-count");
                outputLine("<place>", element->name(), "</place>");
            }
        }

        void XMLPrinter::_accept(const LiteralExpr *element) {
            outputLine("<integer-constant>", element->value(), "</integer-constant>");
        }

        void XMLPrinter::_accept(const PlusExpr *element) {
            if (token_count) {
                for(auto& e : element->expressions())
                    e->visit(*this);
                return;
            }

            if(element->tk) {
                Tag t(this, "tokens-count");
                for(auto& e : element->places())
                    outputLine("<place>", e.second, "</place>");
                for(auto& e : element->expressions())
                    e->visit(*this);
                return;
            }
            {
                Tag t(this, "integer-sum");
                outputLine("<integer-constant>", element->constant(), "</integer-constant>");
                for(auto& i : element->places())
                {
                    Tag tc(this, "tokens-count");
                    outputLine("<place>", i.second, "</place>");
                }
                for(auto& e : element->expressions())
                    e->visit(*this);
            }
        }

        void XMLPrinter::_accept(const MultiplyExpr *element) {
            Tag i(this, "integer-product");
            for(auto& e : element->expressions())
                e->visit(*this);
        }

        void XMLPrinter::_accept(const MinusExpr *element) {
            Tag i(this, "integer-difference");
            outputLine("<integer-constant>0</integer-constant>");
            (*element)[0]->visit(*this);
        }

        void XMLPrinter::_accept(const SubtractExpr *element) {
            Tag i(this, "integer-difference");
            for(auto& e : element->expressions())
                e->visit(*this);
        }

        void XMLPrinter::_accept(const IdentifierExpr *element) {
            element->compiled()->visit(*this);
        }

        void XMLPrinter::_accept(const ShallowCondition *element) {
            element->getCompiled()->visit(*this);
        }
    }
}

