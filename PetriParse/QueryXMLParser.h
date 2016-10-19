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
#include <sstream> 

#include "PNMLParser.h"
#include "rapidxml/rapidxml.hpp"

using namespace std;

class QueryXMLParser {
public:
    QueryXMLParser(const PNMLParser::TransitionEnablednessMap &transitionEnabledness);
    ~QueryXMLParser();

    struct QueryItem {
        string id; // query name
        string queryText; // only EF queries will be here
        bool negateResult; // true if the final result should be negated
        std::vector<std::string> boundNames;

        enum {
            PARSING_OK,
            UNSUPPORTED_QUERY,
        } parsingResult;
    };

    typedef vector<QueryItem>::iterator QueryIterator;
    vector<QueryItem>  queries;

    bool parse(std::ifstream& xml);
    void printQueries();
    void printQueries(size_t i);

private:
    bool parsePropertySet(rapidxml::xml_node<>* element);
    bool parseProperty(rapidxml::xml_node<>*  element);
    bool parseTags(rapidxml::xml_node<>*  element);
    bool parseFormula(rapidxml::xml_node<>*  element, string &queryText, bool &negateResult, std::vector<string> &boundNames);
    bool parseBooleanFormula(rapidxml::xml_node<>*  element, string &queryText);
    bool parseIntegerExpression(rapidxml::xml_node<>*  element, string &queryText);
    string parsePlace(rapidxml::xml_node<>*  element);
    PNMLParser::TransitionEnablednessMap _transitionEnabledness;
};

#endif /* QUERYXMLPARSER_H */

