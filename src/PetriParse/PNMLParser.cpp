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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <cstring>


#include "PetriParse/PNMLParser.h"
#include "PetriEngine/errorcodes.h"

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
        std::cerr << "expecting <pnml> tag as root-node in xml tree." << std::endl;
        exit(ErrorCode);
    }
    
    auto declarations = root->first_node("net")->first_node("declaration");
    isColored = declarations != nullptr;
    if (isColored) {
        builder->enableColors();
        parseDeclarations(declarations);
    }
    parseElement(root);

    //Add all the transition
    for (auto & transition : transitions)
        if (!isColored) {
            builder->addTransition(transition.id, transition.x, transition.y);
        } else {
            builder->addTransition(transition.id, transition.expr, transition.x, transition.y);
        }

    //Add all the arcs
    for (auto & arc : arcs) {
        auto a = arc;
        
        //Check that source id exists
        if (id2name.find(arc.source) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc source with id=\"%s\" wasn't found!\n",
                    arc.source.c_str());
            continue;
        }
        //Check that target id exists
        if (id2name.find(arc.target) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc target with id=\"%s\" wasn't found!\n",
                    arc.target.c_str());
            continue;
        }
        //Find source and target
        NodeName source = id2name[arc.source];
        NodeName target = id2name[arc.target];

        if (source.isPlace && !target.isPlace) {
            if (!isColored) {
                builder->addInputArc(source.id, target.id, false, arc.weight);
            } else {
                builder->addInputArc(source.id, target.id, arc.expr);
            }

        } else if (!source.isPlace && target.isPlace) {
            if (!isColored) {
                builder->addOutputArc(source.id, target.id, arc.weight);
            } else {
                builder->addOutputArc(source.id, target.id, arc.expr);
            }
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
    this->builder = nullptr;

    //Cleanup
    id2name.clear();
    arcs.clear();
    transitions.clear();
    inhibarcs.clear();
    colorTypes.clear();
    builder->sort();
}

void PNMLParser::parseDeclarations(rapidxml::xml_node<>* element) {
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "namedsort") == 0) {
            parseNamedSort(it);
        } else if (strcmp(it->name(), "variabledecl") == 0) {
            auto var = new PetriEngine::Colored::Variable {
                it->first_attribute("id")->value(),
                parseUserSort(it)
            };
            variables[it->first_attribute("id")->value()] = var;
        } else {
            parseDeclarations(element->first_node());
        }
    }
}

bool isInitialBinding(std::vector<const PetriEngine::Colored::Color*>& binding) {
    for (auto color : binding) {
        if (color->getId() != 0)
            return false;
    }
    return true;
}

void PNMLParser::parseNamedSort(rapidxml::xml_node<>* element) {
    auto type = element->first_node();
    auto ct = strcmp(type->name(), "productsort") == 0 ?
              new PetriEngine::Colored::ProductType(std::string(element->first_attribute("id")->value())) :
              new PetriEngine::Colored::ColorType(std::string(element->first_attribute("id")->value()));
    
    if (strcmp(type->name(), "dot") == 0) {
        ct->addDot();
    } else if (strcmp(type->name(), "productsort") == 0) {
        for (auto it = type->first_node(); it; it = it->next_sibling()) {
            if (strcmp(it->name(), "usersort") == 0) {
                ((PetriEngine::Colored::ProductType*)ct)->addType(colorTypes[it->first_attribute("declaration")->value()]);
            }
        }
    } else {
        for (auto it = type->first_node(); it; it = it->next_sibling()) {
            auto id = it->first_attribute("id");
            assert(id != nullptr);
            ct->addColor(id->value());
        }
    }

    std::string id = element->first_attribute("id")->value();
    colorTypes[id] = ct;
    builder->addColorType(id, ct);
}

PetriEngine::Colored::ArcExpression_ptr PNMLParser::parseArcExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "numberof") == 0) {
        return parseNumberOfExpression(element);
    } else if (strcmp(element->name(), "add") == 0) {
        std::vector<PetriEngine::Colored::ArcExpression_ptr> constituents;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            constituents.push_back(parseArcExpression(it));
        }
        return std::make_shared<PetriEngine::Colored::AddExpression>(std::move(constituents));
    } else if (strcmp(element->name(), "subtract") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        auto res = std::make_shared<PetriEngine::Colored::SubtractExpression>(parseArcExpression(left), parseArcExpression(right));
        auto next = right;
        while ((next = next->next_sibling())) {
            res = std::make_shared<PetriEngine::Colored::SubtractExpression>(res, parseArcExpression(next));
        }
        return res;
    } else if (strcmp(element->name(), "scalarproduct") == 0) {
        auto scalar = element->first_node();
        auto ms = scalar->next_sibling();
        return std::make_shared<PetriEngine::Colored::ScalarProductExpression>(parseArcExpression(ms), parseNumberConstant(scalar));
    } else if (strcmp(element->name(), "all") == 0) {
        return parseNumberOfExpression(element->parent());
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseArcExpression(element->first_node());
    }
    printf("Could not parse '%s' as an arc expression\n", element->name());
    assert(false);
    return nullptr;
}

PetriEngine::Colored::GuardExpression_ptr PNMLParser::parseGuardExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "lt") == 0 || strcmp(element->name(), "lessthan") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::LessThanExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "gt") == 0 || strcmp(element->name(), "greaterthan") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::GreaterThanExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "leq") == 0 || strcmp(element->name(), "lessthanorequal") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::LessThanEqExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "geq") == 0 || strcmp(element->name(), "greaterthanorequal") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::GreaterThanEqExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "eq") == 0 || strcmp(element->name(), "equality") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::EqualityExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "neq") == 0 || strcmp(element->name(), "inequality") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::InequalityExpression>(parseColorExpression(left), parseColorExpression(right));
    } else if (strcmp(element->name(), "not") == 0) {
        return std::make_shared<PetriEngine::Colored::NotExpression>(parseGuardExpression(element->first_node()));
    } else if (strcmp(element->name(), "and") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::AndExpression>(parseGuardExpression(left), parseGuardExpression(right));
    } else if (strcmp(element->name(), "or") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        return std::make_shared<PetriEngine::Colored::OrExpression>(parseGuardExpression(left), parseGuardExpression(right));
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseGuardExpression(element->first_node());
    }
    
    printf("Could not parse '%s' as a guard expression\n", element->name());
    assert(false);
    return nullptr;
}

PetriEngine::Colored::ColorExpression_ptr PNMLParser::parseColorExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "dotconstant") == 0) {
        return std::make_shared<PetriEngine::Colored::DotConstantExpression>();
    } else if (strcmp(element->name(), "variable") == 0) {
        return std::make_shared<PetriEngine::Colored::VariableExpression>(variables[element->first_attribute("refvariable")->value()]);
    } else if (strcmp(element->name(), "useroperator") == 0) {
        return std::make_shared<PetriEngine::Colored::UserOperatorExpression>(findColor(element->first_attribute("declaration")->value()));
    } else if (strcmp(element->name(), "successor") == 0) {
        return std::make_shared<PetriEngine::Colored::SuccessorExpression>(parseColorExpression(element->first_node()));
    } else if (strcmp(element->name(), "predecessor") == 0) {
        return std::make_shared<PetriEngine::Colored::PredecessorExpression>(parseColorExpression(element->first_node()));
    } else if (strcmp(element->name(), "tuple") == 0) {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            colors.push_back(parseColorExpression(it));
        }
        return std::make_shared<PetriEngine::Colored::TupleExpression>(std::move(colors));
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseColorExpression(element->first_node());
    }
    assert(false);
    return nullptr;
}

PetriEngine::Colored::AllExpression_ptr PNMLParser::parseAllExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "all") == 0) {
        return std::make_shared<PetriEngine::Colored::AllExpression>(parseUserSort(element));
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseAllExpression(element->first_node());
    }
    
    return nullptr;
}

PetriEngine::Colored::ColorType* PNMLParser::parseUserSort(rapidxml::xml_node<>* element) {
    if (element) {
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            if (strcmp(it->name(), "usersort") == 0) {
                return colorTypes[it->first_attribute("declaration")->value()];
            } else if (strcmp(it->name(), "structure") == 0
                    || strcmp(it->name(), "type") == 0
                    || strcmp(it->name(), "subterm") == 0) {
                return parseUserSort(it);
            }
        }
    }
    assert(false);
    return nullptr;
}

PetriEngine::Colored::NumberOfExpression_ptr PNMLParser::parseNumberOfExpression(rapidxml::xml_node<>* element) {
    auto num = element->first_node();
    uint32_t number = parseNumberConstant(num);
    rapidxml::xml_node<>* first;
    if (number) {
        first = num->next_sibling();
    } else {
        number = 1;
        first = num;
    }
    auto allExpr = parseAllExpression(first);
    if (allExpr) {
        return std::make_shared<PetriEngine::Colored::NumberOfExpression>(std::move(allExpr), number);
    } else {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
        for (auto it = first; it; it = it->next_sibling()) {
            colors.push_back(parseColorExpression(it));
        }
        return std::make_shared<PetriEngine::Colored::NumberOfExpression>(std::move(colors), number);
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
            std::cerr << "variable not supported" << std::endl;
            exit(ErrorCode);
        } else if (strcmp(it->name(),"queries") == 0) {
            parseQueries(it);
        } else if (strcmp(it->name(), "k-bound") == 0) {
            std::cerr << "k-bound should be given as command line option -k" << std::endl;
            exit(ErrorCode);
        } else if (strcmp(it->name(),"query") == 0) {
            std::cerr << "query tag not supported, please use PQL or XML-style queries instead" << std::endl;
            exit(ErrorCode);            
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
    
    auto initial = element->first_attribute("initialMarking");
    long long initialMarking = 0;
    PetriEngine::Colored::Multiset hlinitialMarking;
    PetriEngine::Colored::ColorType* type = nullptr;
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
        } else if (strcmp(it->name(),"hlinitialMarking") == 0) {
            std::unordered_map<std::string, const PetriEngine::Colored::Color*> binding;
            PetriEngine::Colored::ExpressionContext context {binding, colorTypes};
            hlinitialMarking = parseArcExpression(it->first_node("structure"))->eval(context);
        } else if (strcmp(it->name(), "type") == 0) {
            type = parseUserSort(it);
        }
    }
    
    if(initialMarking > std::numeric_limits<int>::max())
    {
        std::cerr << "Number of tokens in " << id << " exceeded " << std::numeric_limits<int>::max() << std::endl;
        exit(ErrorCode);
    }
    //Create place
    if (!isColored) {
        builder->addPlace(id, initialMarking, x, y);
    } else {
        if (!type) {
            std::cerr << "Place '" << id << "' is missing color type" << std::endl;
            exit(ErrorCode);
        }
        else
        {
            builder->addPlace(id, type, std::move(hlinitialMarking), x, y);
        }
    }
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
        std::cerr << "timed arcs are not supported" << std::endl;
        exit(ErrorCode);
    }
    else if(type && strcmp(type->value(), "inhibitor") == 0)
    {
        inhibitor = true;
    }

    bool first = true;
    for (auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")) {
        string text;
        parseValue(it, text);
        weight = atoi(text.c_str());
        if(std::find_if(text.begin(), text.end(), [](char c) { return !std::isdigit(c) && !std::isblank(c); }) != text.end())
        {
            std::cerr << "ERROR: Found non-integer-text in inscription-tag (weight) on arc from " << source << " to " << target << " with value \"" << text << "\". An integer was expected." << std::endl;
            exit(ErrorCode);
        }
        assert(weight > 0);
        if(!first)
        {
            std::cerr << "ERROR: Multiple inscription tags in xml of a arc from " << source << " to " << target << "." << std::endl;
            exit(ErrorCode);
        }
        first = false;
    }
    
    PetriEngine::Colored::ArcExpression_ptr expr;
    first = true;
    for (auto it = element->first_node("hlinscription"); it; it = it->next_sibling("hlinscription")) {
        expr = parseArcExpression(it->first_node("structure"));
        if(!first)
        {
            std::cerr << "ERROR: Multiple hlinscription tags in xml of a arc from " << source << " to " << target << "." << std::endl;
            exit(ErrorCode);            
        }
        first = false;
    }
    
    if (isColored)
        assert(expr != nullptr);
    Arc arc;
    arc.source = source;
    arc.target = target;
    arc.weight = weight;
    arc.expr = expr;
    assert(weight > 0);
    
    if (inhibitor && isColored) {
        std::cerr << "inhibitor arcs are not supported in colored Petri nets" << std::endl;
        exit(ErrorCode);
    }
    
    if(weight != 0)
    {
        if(inhibitor)
        {
            inhibarcs.push_back(arc);   
        }
        else
        {
            arcs.push_back(arc);
        }
    }
    else
    {
        std::cerr << "ERROR: Arc from " << source << " to " << target << " has non-sensible weight 0." << std::endl;         
        exit(ErrorCode);
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
    t.expr = nullptr;


    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parsePosition(it, t.x, t.y);
        } else if (strcmp(it->name(), "condition") == 0) {
            t.expr = parseGuardExpression(it->first_node("structure"));
        } else if (strcmp(it->name(), "conditions") == 0) {
            std::cerr << "conditions not supported" << std::endl;
            exit(ErrorCode);
        } else if (strcmp(it->name(), "assignments") == 0) {
            std::cerr << "assignments not supported" << std::endl;
            exit(ErrorCode);
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

uint32_t PNMLParser::parseNumberConstant(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "numberconstant") == 0) {
        auto value = element->first_attribute("value")->value();
        return (uint32_t)atoll(value);
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseNumberConstant(element->first_node());
    }
    return 0;
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

const PetriEngine::Colored::Color* PNMLParser::findColor(const char* name) const {
    for (const auto& elem : colorTypes) {
        auto col = (*elem.second)[name];
        if (col)
            return col;
    }
    printf("Could not find color: %s\nCANNOT_COMPUTE\n", name);
    exit(ErrorCode);
}
