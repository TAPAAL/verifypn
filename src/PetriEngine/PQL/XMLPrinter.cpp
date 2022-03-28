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
                Visitor::visit(this, c);
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
            Visitor::visit(this, element->getCond());
        }

        void XMLPrinter::_accept(const AndCondition *element) {
            if(element->empty())
            {
                Visitor::visit(this, BooleanCondition::TRUE_CONSTANT);
                return;
            }
            if(element->size() == 1)
            {
                Visitor::visit(this, (*element)[0]);
                return;
            }
            {
                Tag t(this, "conjunction");
                for(auto& e : *element)
                    Visitor::visit(this, e);
            }
        }

        void XMLPrinter::_accept(const OrCondition *element) {
            if(element->empty())
            {
                Visitor::visit(this, BooleanCondition::FALSE_CONSTANT);
                return;
            }
            if(element->size() == 1)
            {
                Visitor::visit(this, (*element)[0]);
                return;
            }
            {
                Tag d(this, "disjunction");
                for(auto& e : *element)
                    Visitor::visit(this, e);
            }
        }

        void XMLPrinter::_accept(const LessThanCondition *element) {
            Tag t(this, "integer-lt");
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }

        void XMLPrinter::_accept(const LessThanOrEqualCondition *element) {
            Tag t(this, "integer-le");
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }

        void XMLPrinter::_accept(const EqualCondition *element) {
            Tag t(this, "integer-eq");
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }

        void XMLPrinter::_accept(const NotEqualCondition *element) {
            Tag t(this, "integer-ne");
            Visitor::visit(this, (*element)[0]);
            Visitor::visit(this, (*element)[1]);
        }

        void XMLPrinter::_accept(const DeadlockCondition *element) {
            outputLine("<deadlock/>");
        }

        void XMLPrinter::_accept(const CompareConjunction *element) {
            if(element->isNegated()) {
                generateTabs() << "<negation>";
            }

            if(element->constraints().empty()) Visitor::visit(this, BooleanCondition::TRUE_CONSTANT);
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
                        outputLine("<place>", *c._name, "</place>");
                        closeXmlTag("tokens-count");
                        outputLine("<integer-constant>", c._lower, "</integer-constant>");
                        closeXmlTag("integer-ge");
                    }
                    if(c._upper != std::numeric_limits<uint32_t>::max())
                    {
                        openXmlTag("integer-le");
                        openXmlTag("tokens-count");
                        outputLine("<place>", *c._name, "</place>");
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
                outputLine("<place>", *p._name, "</place>");
            }
        }

        void XMLPrinter::_accept(const ControlCondition *condition) {
            Tag t(this, "control");
            Visitor::visit(this, (*condition)[0]);
        }

        void XMLPrinter::_accept(const EFCondition *condition) {
            Tag ep(this, "exists-path");
            {
                Tag f(this, "finally");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const EGCondition *condition) {
            Tag ep(this, "exists-path");
            {
                Tag g(this, "globally");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const AGCondition *condition) {
            Tag a(this, "all-paths");
            {
                Tag g(this, "globally");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const AFCondition *condition) {
            Tag a(this, "all-paths");
            {
                Tag f(this, "finally");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const EXCondition *condition) {
            Tag e(this, "exists-path");
            {
                Tag n(this, "next");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const AXCondition *condition) {
            Tag a(this, "all-paths");
            {
                Tag e(this, "next");
                Visitor::visit(this, condition->getCond());
            }
        }

        void XMLPrinter::_accept(const EUCondition *condition) {
            Tag e(this, "exists-path");
            {
                Tag u(this, "until");
                {
                    Tag b(this, "before");
                    Visitor::visit(this, (*condition)[0]);
                }
                {
                    Tag r(this, "reach");
                    Visitor::visit(this, (*condition)[1]);
                }
            }
        }

        void XMLPrinter::_accept(const AUCondition *condition) {
            Tag e(this, "all-paths");
            {
                Tag u(this, "until");
                {
                    Tag b(this, "before");
                    Visitor::visit(this, (*condition)[0]);
                }
                {
                    Tag r(this, "reach");
                    Visitor::visit(this, (*condition)[1]);
                }
            }
        }

        void XMLPrinter::_accept(const ACondition *condition) {
            Tag a(this, "all-paths");
            Visitor::visit(this, condition->getCond());
        }

        void XMLPrinter::_accept(const ECondition *condition) {
            Tag e(this, "exists-path");
            Visitor::visit(this, condition->getCond());
        }

        void XMLPrinter::_accept(const GCondition *condition) {
            Tag g(this, "globally");
            Visitor::visit(this, condition->getCond());
        }

        void XMLPrinter::_accept(const FCondition *condition) {
            Tag f(this, "finally");
            Visitor::visit(this, condition->getCond());
        }

        void XMLPrinter::_accept(const XCondition *condition) {
            Tag n(this, "next");
            Visitor::visit(this, condition->getCond());
        }

        void XMLPrinter::_accept(const UntilCondition *condition) {
            Tag u(this, "until");
            {
                {
                    Tag b(this, "before");
                    Visitor::visit(this, (*condition)[0]);
                }
                {
                    Tag r(this, "reach");
                    Visitor::visit(this, (*condition)[1]);
                }
            }
        }

        void XMLPrinter::_accept(const UnfoldedFireableCondition *element) {
            outputLine("<is-fireable><transition>", *element->getName(), "</transition></is-fireable>");
        }

        void XMLPrinter::_accept(const BooleanCondition *element) {
            outputLine(element->value ? "<true/>": "<false/>");
        }

        void XMLPrinter::_accept(const UnfoldedIdentifierExpr *element) {
            Tag tc(this, "tokens-count");
            outputLine("<place>", *element->name(), "</place>");
        }

        void XMLPrinter::_accept(const LiteralExpr *element) {
            outputLine("<integer-constant>", element->value(), "</integer-constant>");
        }

        void XMLPrinter::_accept(const PlusExpr *element) {
            auto printer = [this](auto* element) {
                if(element->constant() != 0)
                    outputLine("<integer-constant>", element->constant(), "</integer-constant>");
                if(!element->places().empty()) {
                    Tag tc(this, "tokens-count");
                    for(auto& i : element->places())
                    {
                        outputLine("<place>", *i.second, "</place>");
                    }
                }
                for(auto& e : element->expressions())
                    Visitor::visit(this, e);
            };
            auto nconst = (element->constant() != 0 ? 1 : 0);
            auto ntoken_count = (element->places().empty() ? 0 : 1);
            if(element->expressions().size() + ntoken_count + nconst >= 2){
                Tag t(this, "integer-sum");
                printer(element);
            }
            else
            {
                printer(element);
            }
        }

        void XMLPrinter::_accept(const MultiplyExpr *element) {
            auto printer = [this](auto* element) {
                if(element->constant() != 1)
                outputLine("<integer-constant>", element->constant(), "</integer-constant>");
                if(!element->places().empty()) {
                    for(auto& i : element->places())
                    {
                        Tag tc(this, "tokens-count");
                        outputLine("<place>", *i.second, "</place>");
                    }
                }
                for(auto& e : element->expressions())
                    Visitor::visit(this, e);
            };

            auto nconst = (element->constant() != 1 ? 1 : 0);
            if(nconst + element->expressions().size() + element->places().size() >= 2)
            {
                Tag i(this, "integer-product");
                printer(element);
            }
            else
            {
                printer(element);
            }
        }

        void XMLPrinter::_accept(const MinusExpr *element) {
            Tag i(this, "integer-difference");
            outputLine("<integer-constant>0</integer-constant>");
            Visitor::visit(this, (*element)[0]);
        }

        void XMLPrinter::_accept(const SubtractExpr *element) {
            Tag i(this, "integer-difference");
            for(auto& e : element->expressions())
                Visitor::visit(this, e);
        }

        void XMLPrinter::_accept(const IdentifierExpr *element) {
            Visitor::visit(this, element->compiled());
        }

        void XMLPrinter::_accept(const ShallowCondition *element) {
            Visitor::visit(this, element->getCompiled());
        }
    }
}

