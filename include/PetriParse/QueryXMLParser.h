/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2014 Jiri Srba <srba.jiri@gmail.com>
 *                    Peter Gjøl Jensen <root@petergjoel.dk>
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
#include <set>

#include <rapidxml.hpp>

#include "PNMLParser.h"
#include "QueryParser.h"
using namespace PetriEngine::PQL;

class QueryXMLParser {
public:
    QueryXMLParser();
    ~QueryXMLParser();

    std::vector<QueryItem>  queries;

    bool parse(std::ifstream& xml, const std::set<size_t>& );
    void printQueries();
    void printQueries(size_t i);

private:
    bool parsePropertySet(rapidxml::xml_node<>* element, const std::set<size_t>&);
    bool parseProperty(rapidxml::xml_node<>*  element);
    bool parseTags(rapidxml::xml_node<>*  element);
    Condition_ptr parseFormula(rapidxml::xml_node<>*  element);
    Condition_ptr parseBooleanFormula(rapidxml::xml_node<>*  element);
    Expr_ptr parseIntegerExpression(rapidxml::xml_node<>*  element);
    string parsePlace(rapidxml::xml_node<>*  element);
    void fatal_error(const std::string& token);
};

#endif /* QUERYXMLPARSER_H */

