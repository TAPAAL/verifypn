/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef PNMLPARSER_H
#define PNMLPARSER_H

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <rapidxml.hpp>

#include "../PetriEngine/AbstractPetriNetBuilder.h"
#include "../PetriEngine/PQL/PQL.h"
#include "../PetriEngine/Colored/ColoredNetStructures.h"
#include "../PetriEngine/Colored/Expressions.h"
#include "../PetriEngine/Colored/Colors.h"

class PNMLParser {

    struct Arc {
        std::string source,
        target;
        int weight;
        PetriEngine::Colored::ArcExpression_ptr expr;
    };
    typedef std::vector<Arc> ArcList;
    typedef ArcList::iterator ArcIter;

    struct Transition {
        std::string id;
        double x, y;
        PetriEngine::Colored::GuardExpression_ptr expr;
    };
    typedef std::vector<Transition> TransitionList;
    typedef TransitionList::iterator TransitionIter;

    struct NodeName {
        std::string id;
        bool isPlace;
    };
    typedef std::unordered_map<std::string, NodeName> NodeNameMap;
    
    typedef std::unordered_map<std::string, PetriEngine::Colored::ColorType*> ColorTypeMap;
    typedef std::unordered_map<std::string, PetriEngine::Colored::Variable*> VariableMap;

public:

    struct Query {
        std::string name, text;
    };
    
    PNMLParser() {
        builder = NULL;
    }
    void parse(std::ifstream& xml,
            PetriEngine::AbstractPetriNetBuilder* builder);

    std::vector<Query> getQueries() {
        return queries;
    }

private:
    void parseElement(rapidxml::xml_node<>* element);
    void parsePlace(rapidxml::xml_node<>* element);
    void parseArc(rapidxml::xml_node<>* element, bool inhibitor = false);
    void parseTransition(rapidxml::xml_node<>* element);
    void parseDeclarations(rapidxml::xml_node<>* element);
    void parseNamedSort(rapidxml::xml_node<>* element);
    PetriEngine::Colored::ArcExpression_ptr parseArcExpression(rapidxml::xml_node<>* element);
    PetriEngine::Colored::GuardExpression_ptr parseGuardExpression(rapidxml::xml_node<>* element);
    PetriEngine::Colored::ColorExpression_ptr parseColorExpression(rapidxml::xml_node<>* element);
    PetriEngine::Colored::AllExpression_ptr parseAllExpression(rapidxml::xml_node<>* element);
    PetriEngine::Colored::ColorType* parseUserSort(rapidxml::xml_node<>* element);
    PetriEngine::Colored::NumberOfExpression_ptr parseNumberOfExpression(rapidxml::xml_node<>* element);
    void parseTransportArc(rapidxml::xml_node<>* element);
    void parseValue(rapidxml::xml_node<>* element, std::string& text);
    uint32_t parseNumberConstant(rapidxml::xml_node<>* element);
    void parsePosition(rapidxml::xml_node<>* element, double& x, double& y);
    void parseQueries(rapidxml::xml_node<>* element);
    const PetriEngine::Colored::Color* findColor(const char* name) const;
    PetriEngine::AbstractPetriNetBuilder* builder;
    NodeNameMap id2name;
    ArcList arcs;
    ArcList inhibarcs;
    TransitionList transitions;
    ColorTypeMap colorTypes;
    VariableMap variables;
    bool isColored;
    std::vector<Query> queries;
};

#endif // PNMLPARSER_H
