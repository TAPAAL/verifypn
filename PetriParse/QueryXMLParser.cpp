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
    stringstream queryText;
    bool negateResult = false;
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
    if (tagsOK && parseFormula(formulaPtr, queryText, negateResult, queryItem.boundNames)) {
        queryItem.queryText = queryText.str();
        queryItem.negateResult = negateResult;
        queryItem.parsingResult = QueryItem::PARSING_OK;
    } else {
        queryItem.queryText = "";
        queryItem.negateResult = false;
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

bool QueryXMLParser::parseFormula(rapidxml::xml_node<>*  element, stringstream &queryText, bool &negateResult,
        std::vector<string> &placeBounds) {
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
        return false;
    }
    rapidxml::xml_node<>*  booleanFormula = element->first_node();
    string elementName = booleanFormula->name();
    if (elementName == "invariant") {
        queryText << "EF not(";
        negateResult = true;
    } else if (elementName == "impossibility") {
        queryText << "EF ( ";
        negateResult = true;
    } else if (elementName == "possibility") {
        queryText << "EF ( ";
        negateResult = false;
    } else if (elementName == "all-paths") { // new A operator for 2015 competition
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return false;
        }
        booleanFormula = children;
        if (booleanFormula && strcmp(booleanFormula->name(), "globally") == 0) {
            queryText << "EF not ( ";
            negateResult = true;
        } else {
            return false;
        }
    } else if (elementName == "exists-path") { // new E operator for 2015 competition
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return false;
        }

        booleanFormula = children;
        if (booleanFormula && strcmp(children->name(), "finally") == 0) {
            queryText << "EF ( ";
            negateResult = false;
        } else {
            return false;
        }
    } else if (elementName == "negation") {
        rapidxml::xml_node<>* children = booleanFormula->first_node();
        if (getChildCount(children) != 1) {
            return false;
        }
        booleanFormula = children->first_node();
        if (!booleanFormula) return false;
        string negElementName = booleanFormula->name();
        if (negElementName == "invariant") {
            queryText << "EF not( ";
            negateResult = false;
        } else if (negElementName == "impossibility") {
            queryText << "EF ( ";
            negateResult = false;
        } else if (negElementName == "possibility") {
            queryText << "EF ( ";
            negateResult = true;
        } else {
            return false;
        }
    } else if (elementName == "place-bound") {
        queryText << "EF true ";
        for(auto it = booleanFormula->first_node(); it ; it = it->next_sibling())
        {
            if (strcmp(it->name(), "place") != 0) {
                return false;
            }
            placeBounds.push_back(parsePlace(it));
            if (placeBounds.back() == "") {
                return false; // invalid place name
            }
            queryText << " and \"" << placeBounds.back() << "\"" << " < 0";
        }
        negateResult = false;
        return true;
    } else {
        return false;
    }
    auto nextElements = booleanFormula->first_node();
    if (nextElements == NULL || getChildCount(booleanFormula) != 1 || !parseBooleanFormula(nextElements, queryText)) {
        return false;
    }
    queryText << " )";
    placeBounds.clear();
    return true;
}

bool QueryXMLParser::parseBooleanFormula(rapidxml::xml_node<>*  element, stringstream &queryText) {
    string elementName = element->name();
    if (elementName == "deadlock") {
        queryText << "deadlock";
        return true;
    } else if (elementName == "true") {
        queryText << "true";
        return true;
    } else if (elementName == "false") {
        queryText << "false";
        return true;
    } else if (elementName == "negation") {
        auto children = element->first_node();
        queryText << "not(";
        if (getChildCount(element) == 1 && parseBooleanFormula(children, queryText)) {
            queryText << ")";
        } else {
            return false;
        }
        return true;
    } else if (elementName == "conjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) {
            return false;
        }
        queryText << "(";
        // skip a sibling
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if(!first) queryText << " and ";
            first = false;
            if (!(parseBooleanFormula(it, queryText))) {
                return false;
            }
        }
        queryText << ")";
        return true;
    } else if (elementName == "disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) {
            return false;
        }
        queryText << "(";

        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if(!first) queryText << " or ";
            first = false;
            if (!(parseBooleanFormula(it, queryText))) {
                return false;
            }
        }
        queryText << ")";
        return true;
    } else if (elementName == "exclusive-disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // we support only two subformulae here
            return false;
        }
        stringstream subformula1;
        stringstream subformula2;
        if (!(parseBooleanFormula(children, subformula1))) {
            return false;
        }
        if (!(parseBooleanFormula(children->next_sibling(), subformula2))) {
            return false;
        }
        queryText << "(((" << subformula1.str() << " and not(" << subformula2.str() << ")) or (not(" << subformula1.str() << ") and " << subformula2.str() << ")))";
        return true;
    } else if (elementName == "implication") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // implication has only two subformulae
            return false;
        }
        queryText << "(not(";
        if (!(parseBooleanFormula(children, queryText))) {
            return false;
        }
        queryText << ") or ( ";
        if (!(parseBooleanFormula(children->next_sibling(), queryText))) {
            return false;
        }
        queryText << " ))";
        return true;
    } else if (elementName == "equivalence") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // we support only two subformulae here
            return false;
        }
        stringstream subformula1;
        stringstream subformula2;
        if (!(parseBooleanFormula(children, subformula1))) {
            return false;
        }
        if (!(parseBooleanFormula(children->next_sibling(), subformula2))) {
            return false;
        }
        queryText << "((" << subformula1.str() << " and " << subformula2.str() << ") or (not(" << subformula1.str() << ") and not(" << subformula2.str() << ")))";
        return true;
    } else if (elementName == "integer-eq" ||
            elementName == "integer-ne" ||
            elementName == "integer-lt" ||
            elementName == "integer-le" ||
            elementName == "integer-gt" ||
            elementName == "integer-ge") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) { // exactly two integer subformulae are required
            return false;
        }
        
        string mathoperator;
        if (elementName == "integer-eq") mathoperator = " == ";
        else if (elementName == "integer-ne") mathoperator = " != ";
        else if (elementName == "integer-lt") mathoperator = " < ";
        else if (elementName == "integer-le") mathoperator = " <= ";
        else if (elementName == "integer-gt") mathoperator = " > ";
        else if (elementName == "integer-ge") mathoperator = " >= ";
        queryText << "(";
        if (!(parseIntegerExpression(children, queryText))) {
            return false;
        }
        queryText << mathoperator;
        if (!(parseIntegerExpression(children->next_sibling(), queryText))) {
            return false;
        }
        queryText << ")";
        return true;
    } else if (elementName == "is-fireable") {
        auto children = element->first_node();
        
        size_t nrOfChildren = getChildCount(element);
        
        if (nrOfChildren == 0) {
            return false;
        }
        if (nrOfChildren > 1) {
            queryText << "(";
        }
        
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if (strcmp(it->name(), "transition") != 0) {
                return false;
            }
            if (!first) {
                queryText << " or ";
            }
            first = false;
            string transitionName = it->value();
            if (_transitionEnabledness.find(transitionName) == _transitionEnabledness.end()) {
                fprintf(stderr,
                        "XML Query Parsing error: Transition id=\"%s\" was not found!\n",
                        transitionName.c_str());
                return false;
            }
            queryText << _transitionEnabledness[transitionName];
        }
        if (nrOfChildren > 1) {
            queryText << ")";
        }
        return true;
    }
    return false;
}

bool QueryXMLParser::parseIntegerExpression(rapidxml::xml_node<>*  element, stringstream &queryText) {
    string elementName = element->name();
    if (elementName == "integer-constant") {
        int i;
        if (sscanf(element->value(), "%d", &i) == EOF) {
            return false; // expected integer at this place
        }
        stringstream ss; //create a stringstream
        ss << i; //add number to the stream
        queryText << ss.str();
        return true;
    } else if (elementName == "tokens-count") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren == 0) {
            return false;
        }
        if (nrOfChildren > 1) {
            queryText << "(";
        }
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if (strcmp(it->name(), "place") != 0) {
                return false;
            }
            if (!first) {
                queryText << " + ";
            }
            first = false;
            string placeName = parsePlace(it);
            if (placeName == "") {
                return false; // invalid place name
            }
            queryText << "\"" << placeName << "\"";
        }
        if (nrOfChildren > 1) {
            queryText << ")";
        }
        return true;
    } else if (elementName == "integer-sum" ||
            elementName == "integer-product") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren < 2) { // at least two integer subexpression are required
            return false;
        }
        string mathoperator;
        if (elementName == "integer-sum") mathoperator = " + ";
        else if (elementName == "integer-product") mathoperator = " * ";
        queryText << "(";
        bool first = true;
        for (auto it = children; it; it = it->next_sibling()) {
            if (!first) {
                queryText << mathoperator;
            }
            first = false;
            if (!parseIntegerExpression(it, queryText)) {
                return false;
            }
        }
        queryText << ")";
        return true;
    } else if (elementName == "integer-difference") {
        auto children = element->first_node();
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren != 2) { // at least two integer subexpression are required
            return false;
        }
        queryText << "(";
        if (!parseIntegerExpression(children, queryText)) {
            return false;
        }
        queryText << " - ";
        if (!parseIntegerExpression(children->next_sibling(), queryText)) {
            return false;
        }
        queryText << ")";
        return true;
    }
    return false;
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
    cout << it.id << ": " << (it.boundNames.size() > 0 ? "\tplace-bound " : "");
    if (it.parsingResult == QueryItem::UNSUPPORTED_QUERY) {
        cout << "\t---------- unsupported query ----------" << endl;
    } else {
        cout << "\t" << (it.negateResult ? "not " : "") << it.queryText << endl;
    }
}

void QueryXMLParser::printQueries() {
    for (size_t i = 1; i <= queries.size(); i++) {
        printQueries(i);
    }
}
