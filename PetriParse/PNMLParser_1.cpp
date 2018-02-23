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

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <limits>
#include <fstream>
#include <string.h>


#include "PNMLParser_1.h"
#include "../PetriEngine/errorcodes.h"
#include "PetriEngine/PQL/Expressions.h"

using namespace PetriEngine;
using namespace std;
using namespace PetriEngine::PQL;

void PNMLParser::parse(ifstream& xml,
        AbstractPetriNetBuilder* builder) {
    //Clear any left overs
    id2name.clear();
    arcs.clear();
    transitions.clear();
    inhibarcs.clear();
    colorTypes.clear();

    //Set the builder
    this->builder = builder;

    //Parser the xml
    rapidxml::xml_document<> doc;
    vector<char> buffer((istreambuf_iterator<char>(xml)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    
    rapidxml::xml_node<>* root = doc.first_node();
    if(strcmp(root->name(), "pnml") != 0)
    {
        std::cout << "expecting <pnml> tag as root-node in xml tree." << std::endl;
        exit(-1);
    }
    
    parseDeclarations(root->first_node("net")->first_node("declaration"));
    parseElement(root);

    //Add all the transition
    for (TransitionIter it = transitions.begin(); it != transitions.end(); it++)
        builder->addTransition(it->id, it->x, it->y);

    //Add all the arcs
    for (ArcIter it = arcs.begin(); it != arcs.end(); it++) {
        //Check that source id exists
        if (id2name.find(it->source) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc source with id=\"%s\" wasn't found!\n",
                    it->source.c_str());
            continue;
        }
        //Check that target id exists
        if (id2name.find(it->target) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc target with id=\"%s\" wasn't found!\n",
                    it->target.c_str());
            continue;
        }
        //Find source and target
        NodeName source = id2name[it->source];
        NodeName target = id2name[it->target];

        if (source.isPlace && !target.isPlace) {
            builder->addInputArc(source.id, target.id, false, it->weight);

            // cout << "ARC: " << source.id << " to " << target.id << " weight " << it->weight << endl;
            auto cond = std::make_shared<GreaterThanOrEqualCondition>(
                            std::make_shared<IdentifierExpr>(source.id),
                            std::make_shared<LiteralExpr>(it->weight)
                        );
        } else if (!source.isPlace && target.isPlace) {
            builder->addOutputArc(source.id, target.id, it->weight);
        } else {
            fprintf(stderr,
                    "XML Parsing error: Arc from \"%s\" to \"%s\" is neither input nor output!\n",
                    source.id.c_str(),
                    target.id.c_str());
        }
    }

    for(Arc& inhibitor : inhibarcs)
    {
        NodeName source = id2name[inhibitor.source];
        NodeName target = id2name[inhibitor.target];
        if (source.isPlace && !target.isPlace) {
            builder->addInputArc(source.id, target.id, true, inhibitor.weight);
            
            auto cond = std::make_shared<LessThanCondition>(
                            std::make_shared<IdentifierExpr>(source.id),
                            std::make_shared<LiteralExpr>(inhibitor.weight)
                        );
        }
        else
        {
            fprintf(stderr,
                    "XML Parsing error: Inhibitor from \"%s\" to \"%s\" is not valid!\n",
                    source.id.c_str(),
                    target.id.c_str());
        }
    }
    
    //Unset the builder
    this->builder = NULL;

    //Cleanup
    id2name.clear();
    arcs.clear();
    transitions.clear();
    inhibarcs.clear();
    colorTypes.clear();
    builder->sort();
}

void PNMLParser::parseDeclarations(rapidxml::xml_node* element) {
    if (!element) {
        return;
    }
    for (auto it = element->first_node(); it; it->next_sibling()) {
        if (strcmp(it->name(), "namedsort") == 0) {
            parseNamedSort(it);
        } else if (strcmp(it->name(), "variabledecl") == 0) {
            variables[it->first_attribute("id")->value()] = it->first_node()->first_attribute("declaration")->value();
        }
    }
}

void PNMLParser::parseNamedSort(rapidxml::xml_node* element) {
    auto type = element->first_node();
    if (strcmp(type->name(), "dot") == 0)
        return;
    
    auto ct = new PetriEngine::Colored::ColorType();
    for (auto it = type->first_node(); it; it->next_sibling()) {
        ct->addColor(it->first_attribute("id")->value());
    }

    colorTypes[type->first_attribute("id")->value()] = ct;
}

PetriEngine::Colored::ArcExpression* PNMLParser::parseArcExpression(rapidxml::xml_node* element) {
    if (strcmp(element->name(), "numberof") == 0) {
        
    } else if (strcmp(element->name(), "add") == 0) {
        
    } else if (strcmp(element->name(), "subtract") == 0) {
        
    } else if (strcmp(element->name(), "scalarproduct") == 0) {
        
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseArcExpression(element->first_node());
    }
}

PetriEngine::Colored::GuardExpression* PNMLParser::parseGuardExpression(rapidxml::xml_node* element) {
    if (strcmp(element->name(), "lt") == 0 || strcmp(element->name(), "lessthan") == 0) {
        
    } else if (strcmp(element->name(), "gt") == 0 || strcmp(element->name(), "greaterthan") == 0) {
        
    } else if (strcmp(element->name(), "leq") == 0 || strcmp(element->name(), "lessthanorequal") == 0) {
        
    } else if (strcmp(element->name(), "geq") == 0 || strcmp(element->name(), "greaterthanorequal") == 0) {
        
    } else if (strcmp(element->name(), "eq") == 0 || strcmp(element->name(), "equality") == 0) {
        
    } else if (strcmp(element->name(), "neq") == 0 || strcmp(element->name(), "inequality") == 0) {
        
    } else if (strcmp(element->name(), "not") == 0) {
        
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseArcExpression(element->first_node());
    }
}

PetriEngine::Colored::ColorExpression* PNMLParser::parseColorExpression(rapidxml::xml_node* element) {
    if (strcmp(element->name(), "dotconstant") == 0) {
        return new PetriEngine::Colored::DotConstantExpression();
    } else if (strcmp(element->name(), "variable") == 0) {
        return new PetriEngine::Colored::VariableExpression(variables[element->first_attribute("refvariable")->value()]);
    } else if (strcmp(element->name(), "useroperator") == 0) {
        return new PetriEngine::Colored::UserOperatorExpression(findColor(element->first_attribute("declaration")->value()));
    } else if (strcmp(element->name(), "successor") == 0) {
        return new PetriEngine::Colored::SuccessorExpression(parseColorExpression(element->first_node()));
    } else if (strcmp(element->name(), "predecessor") == 0) {
        return new PetriEngine::Colored::PredecessorExpression(parseColorExpression(element->first_node()));
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseColorExpression(element->first_node());
    }
}

void PNMLParser::parseElement(rapidxml::xml_node<>* element) {
    
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "place") == 0) {
            parsePlace(it);
        } else if (strcmp(it->name(),"transition") == 0) {
            parseTransition(it);
        } else if ( strcmp(it->name(),"arc") == 0 ||
                    strcmp(it->name(), "inputArc") == 0 ||
                    strcmp(it->name(), "outputArc") == 0) {
            parseArc(it);
        } else if (strcmp(it->name(),"transportArc") == 0) {
            parseTransportArc(it);
        } else if (strcmp(it->name(),"inhibitorArc") == 0) {
            parseArc(it, true);
        } else if (strcmp(it->name(), "variable") == 0) {
            std::cout << "variable not supported" << std::endl;
            exit(-1);
        } else if (strcmp(it->name(),"queries") == 0) {
            parseQueries(it);
        } else if (strcmp(it->name(), "k-bound") == 0) {
            std::cout << "k-bound should be given as command line option -k" << std::endl;
            exit(-1);
        } else if (strcmp(it->name(),"query") == 0) {
            std::cout << "query tag not supported, please use PQL or XML-style queries instead" << std::endl;
            exit(-1);            
        }
        else
        {
            parseElement(it);
        }
    }
}

void PNMLParser::parseQueries(rapidxml::xml_node<>* element) {
    string query;

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        string name(element->first_attribute("name")->value());
        parseValue(element, query);
        Query q;
        q.name = name;
        q.text = query;
        this->queries.push_back(q);
    }
}

void PNMLParser::parsePlace(rapidxml::xml_node<>* element) {
    double x = 0, y = 0;
    string id(element->first_attribute("id")->value());

    auto hlinitial = element->first_node("hlinitialMarking");
    if (hlinitial) {
        auto hlinitialMarking = parseArcExpression(hlinitial->first_node());
    }
    
    auto initial = element->first_attribute("initialMarking");
    long long initialMarking = 0;
    if(initial)
         initialMarking = atoll(initial->value());

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parsePosition(it, x, y);
        } else if (strcmp(it->name(),"initialMarking") == 0) {
            string text;
            parseValue(it, text);
            initialMarking = atoll(text.c_str());
        }
    }
    
    if(initialMarking > std::numeric_limits<int>::max())
    {
        std::cerr << "Number of tokens in " << id << " exceeded " << std::numeric_limits<int>::max() << std::endl;
        exit(ErrorCode);
    }
    //Create place
    builder->addPlace(id, initialMarking, x, y);
    //Map id to name
    NodeName nn;
    nn.id = id;
    nn.isPlace = true;
    id2name[id] = nn;
}

void PNMLParser::parseArc(rapidxml::xml_node<>* element, bool inhibitor) {
    string source = element->first_attribute("source")->value(),
            target = element->first_attribute("target")->value();
    int weight = 1;
    auto type = element->first_attribute("type");
    if(type && strcmp(type->value(), "timed") == 0)
    {
        std::cout << "timed arcs are not supported" << std::endl;
        exit(-1);
    }
    else if(type && strcmp(type->value(), "inhibitor") == 0)
    {
        inhibitor = true;
    }

    for (auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")) {
            string text;
            parseValue(it, text);
            weight = atoi(text.c_str());
    }
    Arc arc;
    arc.source = source;
    arc.target = target;
    arc.weight = weight;
    if(inhibitor)
    {
        inhibarcs.push_back(arc);   
    }
    else
    {
        arcs.push_back(arc);
    }
}

void PNMLParser::parseTransportArc(rapidxml::xml_node<>* element){
    string source	= element->first_attribute("source")->value(),
           transiton	= element->first_attribute("transition")->value(),
           target	= element->first_attribute("target")->value();
    int weight = 1;

    for(auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")){
        string text;
        parseValue(it, text);
        weight = atoi(text.c_str());
    }

    Arc inArc;
    inArc.source = source;
    inArc.target = transiton;
    inArc.weight = weight;
    arcs.push_back(inArc);

    Arc outArc;
    outArc.source = transiton;
    outArc.target = target;
    outArc.weight = weight;
    arcs.push_back(outArc);
}

void PNMLParser::parseTransition(rapidxml::xml_node<>* element) {
    Transition t;
    t.x = 0;
    t.y = 0;
    t.id = element->first_attribute("id")->value();


    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parsePosition(it, t.x, t.y);
        } else if (strcmp(it->name(), "conditions") == 0) {
            std::cout << "conditions not supported" << std::endl;
            exit(-1);
        } else if (strcmp(it->name(), "assignments") == 0) {
            std::cout << "assignments not supported" << std::endl;
            exit(-1);
        }
    }
    //Add transition to list
    transitions.push_back(t);
    //Map id to name
    NodeName nn;
    nn.id = t.id;
    nn.isPlace = false;
    id2name[t.id] = nn;
}

void PNMLParser::parseValue(rapidxml::xml_node<>* element, string& text) {
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "value") == 0 || strcmp(it->name(), "text") == 0) {
            text = it->value();
        } else
            parseValue(it, text);
    }
}

void PNMLParser::parsePosition(rapidxml::xml_node<>* element, double& x, double& y) {
    for (auto it = element->first_node(); it; it = it->first_node()) {
        if (strcmp(it->name(), "position") == 0) {
            x = atof(it->first_attribute("x")->value());
            y = atof(it->first_attribute("y")->value());
        } else
        {
            parsePosition(it, x, y);
        }
    }
}
