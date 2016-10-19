/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2014 Jiri Srba <srba.jiri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "QueryXMLParser.h"
#include "PetriEngine/PQL/Expressions.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream> 
#include <algorithm> 

using namespace std;

int getChildCount(rapidxml::xml_node<> *n)
{
  int c = 0;
  for (rapidxml::xml_node<> *child = n->first_node(); child != NULL; child = child->next_sibling())
  {
    c++;
  } 
  return c;
} 

QueryXMLParser::QueryXMLParser(const PNMLParser::TransitionEnablednessMap &transitionEnabledness) {
    _transitionEnabledness = transitionEnabledness;
}

QueryXMLParser::~QueryXMLParser() { }

bool QueryXMLParser::parse(std::ifstream& xml) {
    //Parse the xml
    rapidxml::xml_document<> doc;
    vector<char> buffer((istreambuf_iterator<char>(xml)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    rapidxml::xml_node<>*  root = doc.first_node();
    bool parsingOK;
    if (root) {
        parsingOK = parsePropertySet(root);
    } else {
        parsingOK = false;
    }

    //Release DOM tree
    return parsingOK;
}

bool QueryXMLParser::parsePropertySet(rapidxml::xml_node<>*  element) {
    if (strcmp(element->name(), "property-set") != 0) {
        fprintf(stderr, "ERROR missing property-set\n");
        return false; // missing property-set element
    }
    
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (!parseProperty(it)) {
            return false;
        };
    }
    return true;
}

bool QueryXMLParser::parseProperty(rapidxml::xml_node<>*  element) {
    if (strcmp(element->name(), "property") != 0) {
        fprintf(stderr, "ERROR missing property\n");
        return false; // unexpected element (only property is allowed)
    }
    string id;
    bool tagsOK = true;
    rapidxml::xml_node<>* formulaPtr = NULL;
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "id") == 0) {
            id = it->value();
        } else if (strcmp(it->name(), "formula") == 0) {
            formulaPtr = it;
        } else if (strcmp(it->name(), "tags") == 0) {
            tagsOK = parseTags(it);
        }
    }

    if (id == "") {
        fprintf(stderr, "ERROR a query with empty id\n");
        return false;
    }

    QueryItem queryItem;
    queryItem.id = id;
    if(tagsOK)
    {
        queryItem.query.reset(parseFormula(formulaPtr));
        queryItem.parsingResult = QueryItem::PARSING_OK;
    } else {
        queryItem.query = NULL;
        queryItem.parsingResult = QueryItem::UNSUPPORTED_QUERY;
    }
    queries.push_back(queryItem);
    return true;
}

bool QueryXMLParser::parseTags(rapidxml::xml_node<>*  element) {
    // we can accept only reachability query
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "is-reachability") == 0 && strcmp(it->value(), "true") == 0) {
            return false;
        }
    }
    return true;
}

Condition* QueryXMLParser::parseFormula(rapidxml::xml_node<>*  element) {
    /*
     Describe here how to parse
     * INV phi =  AG phi =  not EF not phi
     * IMPOS phi = AG not phi = not EF phi
     * POS phi = EF phi
     * NEG INV phi = not AG phi = EF not phi
     * NEG IMPOS phi = not AG not phi = EF phi
     * NEG POS phi = not EF phi
     */
    if (getChildCount(element) != 1) {
        return NULL;
    }
    bool negateResult = false;
    bool addNot = false;
    rapidxml::xml_node<>*  booleanFormula = element->first_node();
    string elementName = booleanFormula->name();
    if (elementName == "invariant") {
        negateResult = true;
        addNot = true;
    } else if (elementName == "impossibility") {
        negateResult = true;
    } else if (elementName == "possibility") {
        negateResult = false;
    } else if (elementName == "all-paths") { // new A operator for 2015 competition
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return NULL;
        }
        booleanFormula = children;
        if (booleanFormula && strcmp(booleanFormula->name(), "globally") == 0) {
            addNot = false;
            negateResult = true;
        } else {
            return NULL;
        }
    } else if (elementName == "exists-path") { // new E operator for 2015 competition
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return NULL;
        }

        booleanFormula = children;
        if (booleanFormula && strcmp(children->name(), "finally") == 0) {
            negateResult = false;
        } else {
            return NULL;
        }
    } else if (elementName == "negation") {
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return NULL;
        }
        booleanFormula = children->first_node();
        if (!booleanFormula) return NULL;
        string negElementName = booleanFormula->name();
        if (negElementName == "invariant") {
            addNot = true;
            negateResult = false;
        } else if (negElementName == "impossibility") {
            negateResult = false;
        } else if (negElementName == "possibility") {
            negateResult = true;
        } else {
            return NULL;
        }
    } else if (elementName == "place-bound") {
        Condition* cond = new BooleanCondition(true);
        for(auto it = booleanFormula->first_node(); it ; it = it->next_sibling())
        {
            if (strcmp(it->name(), "place") != 0) {
                return NULL;
            }
            auto place = parsePlace(it);
            if (place == "") {
                return NULL; // invalid place name
            }
            cond = new AndCondition(cond, 
                    new LessThanCondition(
                        new IdentifierExpr(place),
                                         0));
        }
        return cond;
    } else {
        return NULL;
    }
    auto nextElements = booleanFormula->first_node();
    if (nextElements != NULL || getChildCount(booleanFormula) == 1) {
        Condition* cond = parseBooleanFormula(nextElements);
        if(addNot && cond) cond = new NotCondition(cond);
        if(cond && negateResult) cond->setInvariant(true);
               
        return cond;
    }
    else
    {
        return NULL;
    }
}

Condition* QueryXMLParser::parseBooleanFormula(rapidxml::xml_node<>*  element) {
    string elementName = element->name();
    if (elementName == "deadlock") {
        return new DeadlockCondition();
    } else if (elementName == "true") {
        return new BooleanCondition(true);
    } else if (elementName == "false") {
        return new BooleanCondition(false);
    } else if (elementName == "negation") {
        auto children = element->first_node();
        if (getChildCount(element) == 1) {
            Condition* cond = parseBooleanFormula(children);
            if(cond != NULL)
            {
                return new NotCondition(cond);
            }
            return cond;
        } else {
            return NULL;
        }
    } else if (elementName == "conjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) {
            return NULL;
        }
        AndCondition* cond = NULL;
        // skip a sibling
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            Condition* child = parseBooleanFormula(it);
            if(child == NULL)
            {
                return NULL;
            }
            if(first)
            {
                cond = new AndCondition(
                        child,
                        new BooleanCondition(true));
                first = false;
            }
            else
            {
                cond = new AndCondition(cond, child);
            }
        }
        return cond;
    } else if (elementName == "disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) {
            return NULL;
        }
        OrCondition* cond = NULL;
        // skip a sibling
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            Condition* child = parseBooleanFormula(it);
            if(child == NULL)
            {
                return NULL;
            }
            if(first)
            {
                cond = new OrCondition(
                        child,
                        new BooleanCondition(true));
                first = false;
            }
            else
            {
                cond = new OrCondition(cond, child);
            }
        }
        return cond;
    } else if (elementName == "exclusive-disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // we support only two subformulae here
            return NULL;
        }
        Condition* cond1 = parseBooleanFormula(children);
        Condition* cond2 = parseBooleanFormula(children->next_sibling());

        // this should be optimized
        Condition* cond11 = parseBooleanFormula(children);
        Condition* cond22 = parseBooleanFormula(children->next_sibling());        
        if (cond1 == NULL || cond2 == NULL) {
            return NULL;
        }
        
        return new OrCondition(
                new AndCondition(cond1, new NotCondition(cond2))                ,
                new AndCondition(new NotCondition(cond11), cond22)                
                );

    } else if (elementName == "implication") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // implication has only two subformulae
            return NULL;
        }

        Condition* cond1 = parseBooleanFormula(children);
        Condition* cond2 = parseBooleanFormula(children->next_sibling());
        if (cond1 == NULL || cond2 == NULL) {
            return NULL;
        }
        
        return new OrCondition(
                new NotCondition(cond1),
                cond2                
                );
    } else if (elementName == "equivalence") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // we support only two subformulae here
            return NULL;
        }
        Condition* cond1 = parseBooleanFormula(children);
        Condition* cond2 = parseBooleanFormula(children->next_sibling());

        Condition* cond11 = parseBooleanFormula(children);
        Condition* cond22 = parseBooleanFormula(children->next_sibling());
        if (cond1 == NULL || cond2 == NULL) {
            return NULL;
        }

        return new OrCondition(
                new AndCondition(cond1, cond2)                ,
                new AndCondition(new NotCondition(cond11), new NotCondition(cond22))                
                );
    } else if (elementName == "integer-eq" ||
            elementName == "integer-ne" ||
            elementName == "integer-lt" ||
            elementName == "integer-le" ||
            elementName == "integer-gt" ||
            elementName == "integer-ge") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // exactly two integer subformulae are required
            return NULL;
        }
        
        Expr* expr1 = parseIntegerExpression(children);
        Expr* expr2 = parseIntegerExpression(children->next_sibling());
        
        if(expr1 == NULL || expr2 == NULL) 
        {
            return NULL;
        }
        
        if (elementName == "integer-eq") return new EqualCondition(expr1, expr2);
        else if (elementName == "integer-ne") return new NotEqualCondition(expr1, expr2);
        else if (elementName == "integer-lt") return new LessThanCondition(expr1, expr2);
        else if (elementName == "integer-le") return new LessThanOrEqualCondition(expr1, expr2);
        else if (elementName == "integer-gt") return new GreaterThanCondition(expr1, expr2);
        else if (elementName == "integer-ge") return new GreaterThanOrEqualCondition(expr1, expr2);
        return NULL;
        
    } else if (elementName == "is-fireable") {
        auto children = element->first_node();
        
        size_t nrOfChildren = getChildCount(element);
        
        if (nrOfChildren == 0) {
            return NULL;
        }
        
        OrCondition* cond = NULL;
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if (strcmp(it->name(), "transition") != 0) {
                return NULL;
            }
            
            string transitionName = it->value();
            if (_transitionEnabledness.find(transitionName) == _transitionEnabledness.end()) {
                fprintf(stderr,
                        "XML Query Parsing error: Transition id=\"%s\" was not found!\n",
                        transitionName.c_str());
                return NULL;
            }

            if (first) {
                cond = new OrCondition(_transitionEnabledness[transitionName], new BooleanCondition(true));
                first = false;
            }
            else
            {
                cond = new OrCondition(_transitionEnabledness[transitionName], cond);                
            }
        }
        return cond;
    }
    return NULL;
}

Expr* QueryXMLParser::parseIntegerExpression(rapidxml::xml_node<>*  element) {
    string elementName = element->name();
    if (elementName == "integer-constant") {
        int i;
        if (sscanf(element->value(), "%d", &i) == EOF) {
            return NULL; // expected integer at this place
        }
        return new LiteralExpr(i);
    } else if (elementName == "tokens-count") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren == 0) {
            return NULL;
        }

        bool first = true;
        
        PlusExpr* sum = NULL;
        
        for (auto it = children; it; it = it->next_sibling()) {
            if (strcmp(it->name(), "place") != 0) {
                return NULL;
            }
            first = false;
            string placeName = parsePlace(it);
            if (placeName == "") {
                return NULL; // invalid place name
            }
            
            if(first)
            {
                sum = new PlusExpr(new LiteralExpr(0), new IdentifierExpr(placeName));
                first = false;
            }
            else
            {
                sum = new PlusExpr(sum, new IdentifierExpr(placeName));
            }            
        }
        return sum;
    } else if (elementName == "integer-sum" ||
            elementName == "integer-product") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren < 2) { // at least two integer subexpression are required
            return NULL;
        }
        bool isMult = false;
        if (elementName == "integer-sum") isMult = false;
        else if (elementName == "integer-product") isMult = true;
        Expr* expr = NULL;
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            Expr* child = parseIntegerExpression(it);
            if(child == NULL)
            {
                return NULL;
            }
            if(first)
            {
                expr = isMult ?
                    (Expr*)new PlusExpr(new LiteralExpr(0), child) :
                    (Expr*)new MultiplyExpr(new LiteralExpr(1), child);
                first = false;
            }
            else
            {
                expr = isMult ? 
                    (Expr*)new PlusExpr(expr, child) :
                    (Expr*)new MultiplyExpr(expr, child);
            }
        }
        return expr;
    } else if (elementName == "integer-difference") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren != 2) { // at least two integer subexpression are required
            return NULL;
        }
        Expr* expr1 = parseIntegerExpression(children);
        Expr* expr2 = parseIntegerExpression(children->next_sibling());
        
        if(expr1 == NULL || expr2 == NULL) 
        {
            return NULL;
        }
        return new SubtractExpr(expr1, expr2);
    }
    return NULL;
}

string QueryXMLParser::parsePlace(rapidxml::xml_node<>*  element) {
    if (strcmp(element->name(), "place")) {
        return ""; // missing place tag
    }
    string placeName = element->value();
    placeName.erase(std::remove_if(placeName.begin(), placeName.end(), ::isspace), placeName.end());
    return placeName;
}

void QueryXMLParser::printQueries(size_t i) {
    //	QueryXMLParser::QueriesIterator it;
    if (i <= 0 || i > queries.size()) {
        cout << "In printQueries the query index is out of scope\n\n";
        return;
    }
    QueryItem it = queries[i - 1];
    cout << it.id << ": " ;
    if (it.parsingResult == QueryItem::UNSUPPORTED_QUERY) {
        cout << "\t---------- unsupported query ----------" << endl;
    } else {
        cout << "\t" << it.query->toString() << std::endl;
    }
}

void QueryXMLParser::printQueries() {
    for (size_t i = 1; i <= queries.size(); i++) {
        printQueries(i);
    }
}
