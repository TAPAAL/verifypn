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

#ifndef QUERYXMLPARSER_H
#define QUERYXMLPARSER_H

#include <map>
#include <string>
#include <string.h>
#include <vector>
#include <memory>
#include <sstream> 

#include "../PetriEngine/PQL/PQL.h"
#include "PNMLParser.h"
#include "rapidxml/rapidxml.hpp"

using namespace std;
using namespace PetriEngine::PQL;

class QueryXMLParser {
public:
    QueryXMLParser(const PNMLParser::TransitionEnablednessMap &transitionEnabledness);
    ~QueryXMLParser();

    struct QueryItem {
        string id; // query name
        std::shared_ptr<Condition> query;
        std::vector<std::string> boundNames;

        enum {
            PARSING_OK,
            UNSUPPORTED_QUERY,
        } parsingResult;
    };

    vector<QueryItem>  queries;

    bool parse(std::ifstream& xml);
    void printQueries();
    void printQueries(size_t i);

private:
    bool parsePropertySet(rapidxml::xml_node<>* element);
    bool parseProperty(rapidxml::xml_node<>*  element);
    bool parseTags(rapidxml::xml_node<>*  element);
    Condition_ptr parseFormula(rapidxml::xml_node<>*  element);
    Condition_ptr parseBooleanFormula(rapidxml::xml_node<>*  element);
    Expr_ptr parseIntegerExpression(rapidxml::xml_node<>*  element);
    string parsePlace(rapidxml::xml_node<>*  element);
    PNMLParser::TransitionEnablednessMap _transitionEnabledness;
};

#endif /* QUERYXMLPARSER_H */

