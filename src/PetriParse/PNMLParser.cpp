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
#include "utils/errors.h"
#include "PetriEngine/Colored/EvaluationVisitor.h"
#include "PetriEngine/Colored/ConstantVisitor.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Colored;

void PNMLParser::parse(std::istream& xml,
        AbstractPetriNetBuilder* builder) {
    //Clear any left overs
    id2name.clear();
    arcs.clear();
    _transitions.clear();
    colorTypes.clear();
    placeTypeContext = "";
    hasPartition = false;

    //Set the builder
    this->builder = builder;

    //Parse the xml
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(xml)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);

    rapidxml::xml_node<>* root = doc.first_node();
    if(strcmp(root->name(), "pnml") != 0)
    {
        throw base_error("expecting <pnml> tag as root-node in xml tree.");
    }

    auto declarations = root->first_node("declaration");
    if(declarations == nullptr){
        declarations = root->first_node("net")->first_node("declaration");
    }

    isColored = declarations != nullptr;
    if (isColored) {
        builder->enableColors();
        parseDeclarations(declarations);
    }

    parseElement(root);

    //Add all the transition
    for (auto & transition : _transitions)
        if (!isColored) {
            builder->addTransition(transition.id, transition._player, transition.x, transition.y);
        } else {
            builder->addTransition(transition.id, transition.expr, transition._player, transition.x, transition.y);
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
                builder->addInputArc(source.id, target.id, arc.inhib, arc.weight);
            } else {
                builder->addInputArc(source.id, target.id, arc.expr, arc.inhib ? arc.weight : 0);
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

    //Unset the builder
    this->builder = nullptr;

    //Cleanup
    id2name.clear();
    arcs.clear();
    _transitions.clear();
    colorTypes.clear();
    builder->sort();
}

void PNMLParser::parseDeclarations(rapidxml::xml_node<>* element) {
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "namedsort") == 0) {
            parseNamedSort(it);
        } else if (strcmp(it->name(), "variabledecl") == 0) {
            auto var = new Variable {
                it->first_attribute("id")->value(),
                parseUserSort(it)
            };
            variables[it->first_attribute("id")->value()] = var;
            builder->addVariable(var);
        } else if (strcmp(it->name(), "partition") == 0) {
            builder->enablePartition();
            hasPartition = true;
            parsePartitions(it);
        } else {
            parseDeclarations(it);
        }
    }

    for(auto missingCTPair : missingCTs){
        if(colorTypes.count(missingCTPair.first) == 0){
            throw base_error("Unable to find colortype ", missingCTPair.first, " used in product type ", missingCTPair.second->getName());
        }
        builder->addToColorType(missingCTPair.second, colorTypes[missingCTPair.first]);
    }
    missingCTs.clear();
}

void PNMLParser::parsePartitions(rapidxml::xml_node<>* element){
    auto partitionCT = parseUserSort(element);
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "partitionelement") == 0) {
            auto id = it->first_attribute("id")->value();
            std::vector<const Color *> colors;
            for(auto partitionElement = it->first_node(); partitionElement; partitionElement = partitionElement->next_sibling()){
                colors.emplace_back(partitionCT->operator[](partitionElement->first_attribute("declaration")->value()));
            }
            partitions.push_back({colors, id});
        }
    }
}

bool isInitialBinding(std::vector<const Color*>& binding) {
    for (auto color : binding) {
        if (color->getId() != 0)
            return false;
    }
    return true;
}

void PNMLParser::parseNamedSort(rapidxml::xml_node<>* element) {
    auto type = element->first_node();
    const ColorType* fct = nullptr;
    if (strcmp(type->name(), "dot") == 0) {
        fct = Colored::ColorType::dotInstance();
    }
    else
    {
        if (strcmp(type->name(), "productsort") == 0) {
            auto ct = new ProductType(std::string(element->first_attribute("id")->value()));
            bool missingType = false;
            for (auto it = type->first_node(); it; it = it->next_sibling()) {
                if (strcmp(it->name(), "usersort") == 0) {
                    auto ctName = it->first_attribute("declaration")->value();
                    if(!missingType && colorTypes.count(ctName)){
                        ct->addType(colorTypes[ctName]);
                    } else {
                        missingType = true;
                        missingCTs.push_back(std::make_pair(ctName, ct));
                    }
                }
            }
            fct = ct;
        }
        else
        {
            auto ct = new ColorType(std::string(element->first_attribute("id")->value()));
            if (strcmp(type->name(), "finiteintrange") == 0) {

                int64_t start = atoll(type->first_attribute("start")->value());
                int64_t end = atoll(type->first_attribute("end")->value());
                for (uint32_t i = start; i <= end; ++i) {
                    ct->addColor(std::to_string(i).c_str());
                }
                fct = ct;
            } else {
                for (auto it = type->first_node(); it; it = it->next_sibling()) {
                    auto id = it->first_attribute("id");
                    auto name = it->first_attribute("name");
                    assert(id != nullptr);
                    ct->addColor(id->value(), name->value());
                }
            }
            fct = ct;
        }
    }

    std::string id = element->first_attribute("id")->value();
    colorTypes[id] = fct;
    builder->addColorType(id, fct);
}

ArcExpression_ptr PNMLParser::parseArcExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "numberof") == 0) {
        return parseNumberOfExpression(element);
    } else if (strcmp(element->name(), "add") == 0) {
        std::vector<ArcExpression_ptr> constituents;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            constituents.push_back(parseArcExpression(it));
        }
        return std::make_shared<AddExpression>(std::move(constituents));
    } else if (strcmp(element->name(), "subtract") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        auto res = std::make_shared<SubtractExpression>(parseArcExpression(left), parseArcExpression(right));
        auto next = right;
        while ((next = next->next_sibling())) {
            res = std::make_shared<SubtractExpression>(res, parseArcExpression(next));
        }
        return res;
    } else if (strcmp(element->name(), "scalarproduct") == 0) {
        auto scalar = element->first_node();
        auto ms = scalar->next_sibling();
        return std::make_shared<ScalarProductExpression>(parseArcExpression(ms), parseNumberConstant(scalar));
    } else if (strcmp(element->name(), "all") == 0) {
        return parseNumberOfExpression(element->parent());
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseArcExpression(element->first_node());
    } else if (strcmp(element->name(), "tuple") == 0) {
        std::vector<std::vector < ColorExpression_ptr>> collectedColors;
        collectColorsInTuple(element, collectedColors);
        auto expr = constructAddExpressionFromTupleExpression(element, collectedColors, 1);
        return expr;
    }
    throw base_error("Could not parse ", element->name(), " as an arc expression");
    return nullptr;
}

ArcExpression_ptr PNMLParser::constructAddExpressionFromTupleExpression(rapidxml::xml_node<>* element, std::vector<std::vector<ColorExpression_ptr>> collectedColors, uint32_t numberof) {
    std::vector<ArcExpression_ptr> numberOfExpressions;
    if (collectedColors.size() < 2) {
        for (const auto& exp : collectedColors[0]) {
            std::vector<ColorExpression_ptr> colors;
            colors.push_back(exp);
            numberOfExpressions.push_back(std::make_shared<NumberOfExpression>(std::move(colors), numberof));
        }
    } else {
        auto initCartesianSet = cartesianProduct(collectedColors[0], collectedColors[1]);
        for (uint32_t i = 2; i < collectedColors.size(); i++) {
            initCartesianSet = cartesianProduct(initCartesianSet, collectedColors[i]);
        }
        for (const auto& set : initCartesianSet) {
            std::vector<ColorExpression_ptr> colors;
            for (const auto& color : set) {
                colors.push_back(color);
            }
            std::shared_ptr<TupleExpression> tupleExpr = std::make_shared<TupleExpression>(std::move(colors), colorTypes);
            std::vector<ColorExpression_ptr> placeholderVector;
            placeholderVector.push_back(tupleExpr);
            numberOfExpressions.push_back(std::make_shared<NumberOfExpression>(std::move(placeholderVector), numberof));
        }
    }
    return std::make_shared<AddExpression>(std::move(numberOfExpressions));
}

std::vector<std::vector<ColorExpression_ptr>> PNMLParser::cartesianProduct(std::vector<ColorExpression_ptr> rightSet, std::vector<ColorExpression_ptr> leftSet) {
    std::vector<std::vector < ColorExpression_ptr>> returnSet;
    for (const auto& expr : rightSet) {
        for (const auto& expr2 : leftSet) {
            returnSet.emplace_back(std::vector<ColorExpression_ptr>{expr, expr2});
        }
    }
    return returnSet;
}

std::vector<std::vector<ColorExpression_ptr>> PNMLParser::cartesianProduct(std::vector<std::vector<ColorExpression_ptr>> rightSet, std::vector<ColorExpression_ptr> leftSet) {
    std::vector<std::vector < ColorExpression_ptr>> returnSet;
    for (const auto& set : rightSet) {
        for (const auto& expr2 : leftSet) {
            auto setCopy = set;
            setCopy.push_back(expr2);
            returnSet.push_back(std::move(setCopy));
        }
    }
    return returnSet;
}

void PNMLParser::collectColorsInTuple(rapidxml::xml_node<>* element, std::vector<std::vector<ColorExpression_ptr>>&collectedColors) {
    if (strcmp(element->name(), "tuple") == 0) {
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            collectColorsInTuple(it->first_node(), collectedColors);
        }
    } else if (strcmp(element->name(), "all") == 0) {
        std::vector<ColorExpression_ptr> expressionsToAdd;
        auto expr = parseAllExpression(element);
        auto constantMap = Colored::ConstantVisitor::get_constants(*expr);
        for (const auto& positionColors : constantMap) {
            for (const auto& color : positionColors.second) {
                expressionsToAdd.push_back(std::make_shared<UserOperatorExpression>(color));
            }
        }
        collectedColors.push_back(expressionsToAdd);
    } else if (strcmp(element->name(), "add") == 0) {
        std::vector<std::vector < ColorExpression_ptr>> intermediateColors;
        std::vector<std::vector < ColorExpression_ptr>> intermediateColors2;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            collectColorsInTuple(it, intermediateColors2);
            if (intermediateColors.empty()) {
                intermediateColors = intermediateColors2;
            } else {
                for (uint32_t i = 0; i < intermediateColors.size(); i++) {
                    intermediateColors[i].insert(intermediateColors[i].end(), intermediateColors2[i].begin(), intermediateColors2[i].end());
                }
            }
        }
        for (auto &colorVec : intermediateColors) {
            collectedColors.push_back(std::move(colorVec));
        }
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        collectColorsInTuple(element->first_node(), collectedColors);
    } else if (strcmp(element->name(), "finiteintrangeconstant") == 0) {
        std::vector<ColorExpression_ptr> expressionsToAdd;
        auto value = element->first_attribute("value")->value();
        auto intRangeElement = element->first_node("finiteintrange");
        const char* start = intRangeElement->first_attribute("start")->value();
        const char* end = intRangeElement->first_attribute("end")->value();
        expressionsToAdd.push_back(std::make_shared<UserOperatorExpression>(findColorForIntRange(value, start, end)));
        collectedColors.push_back(expressionsToAdd);
    } else if (strcmp(element->name(), "useroperator") == 0 || strcmp(element->name(), "dotconstant") == 0 || strcmp(element->name(), "variable") == 0
            || strcmp(element->name(), "successor") == 0 || strcmp(element->name(), "predecessor") == 0) {
        std::vector<ColorExpression_ptr> expressionsToAdd = findPartitionColors(element);
        if (expressionsToAdd.empty()) {
            auto colors = parseColorExpression(element);
            assert(colors.size() == 1);
            expressionsToAdd.insert(expressionsToAdd.end(), colors.begin(), colors.end());
        }
        collectedColors.push_back(expressionsToAdd);
    } else {
        throw base_error("Could not parse '", element->name(), " as an arc expression when collecting tuple colors\n");
    }
}

GuardExpression_ptr PNMLParser::parseGuardExpression(rapidxml::xml_node<>* element, bool notFlag) {
    if (strcmp(element->name(), "not") == 0) {
        return parseGuardExpression(element->first_node(), true);
    }
    else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseGuardExpression(element->first_node(), notFlag);
    } else if (strcmp(element->name(), "and") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        if (right == nullptr)
            return parseGuardExpression(left, notFlag);
        if (notFlag) {
            return std::make_shared<OrExpression>(parseGuardExpression(left, true), parseGuardExpression(right, true));
        } else {
            return std::make_shared<AndExpression>(parseGuardExpression(left, false), parseGuardExpression(right, false));
        }
    } else if (strcmp(element->name(), "or") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        //There must only be one constituent
        if (right == nullptr) {
            return parseGuardExpression(left, notFlag);
        }
        if (notFlag) {
            auto parentAnd = std::make_shared<AndExpression>(parseGuardExpression(left, true), parseGuardExpression(right, true));
            for (auto it = right->next_sibling(); it; it = it->next_sibling()) {
                parentAnd = std::make_shared<AndExpression>(parentAnd, parseGuardExpression(it, true));
            }
            return parentAnd;
        } else {
            auto parentOr = std::make_shared<OrExpression>(parseGuardExpression(left, false), parseGuardExpression(right, false));
            for (auto it = right->next_sibling(); it; it = it->next_sibling()) {
                parentOr = std::make_shared<OrExpression>(parentOr, parseGuardExpression(it, false));
            }
            return parentOr;
        }
    }
    else
    {
        auto left = element->first_node();
        auto right = left->next_sibling();
        auto lcv = parseColorExpression(left);
        auto rcv = parseColorExpression(right);
        assert(lcv.size() == 1);
        assert(rcv.size() == 1);
        auto& lc = lcv[0];
        auto& rc = rcv[0];
        if (strcmp(element->name(), "lt") == 0 || strcmp(element->name(), "lessthan") == 0) {
            if (notFlag) {
                return std::make_shared<LessThanEqExpression>(std::move(rc), std::move(lc));
            } else {
                return std::make_shared<LessThanExpression>(std::move(lc), std::move(rc));
            }
        } else if (strcmp(element->name(), "gt") == 0 || strcmp(element->name(), "greaterthan") == 0) {
            if (notFlag) {
                return std::make_shared<LessThanEqExpression>(std::move(lc), std::move(rc));
            } else {
                return std::make_shared<LessThanExpression>(std::move(rc), std::move(lc));
            }
        } else if (strcmp(element->name(), "leq") == 0 || strcmp(element->name(), "lessthanorequal") == 0) {
            if (notFlag) {
                return std::make_shared<LessThanExpression>(std::move(rc), std::move(lc));
            } else {
                return std::make_shared<LessThanEqExpression>(std::move(lc), std::move(rc));
            }
        } else if (strcmp(element->name(), "geq") == 0 || strcmp(element->name(), "greaterthanorequal") == 0) {
            if (notFlag) {
                return std::make_shared<LessThanExpression>(std::move(lc), std::move(rc));
            } else {
                return std::make_shared<LessThanEqExpression>(std::move(rc), std::move(lc));
            }
        } else if (strcmp(element->name(), "eq") == 0 || strcmp(element->name(), "equality") == 0) {
            if (notFlag) {
                return std::make_shared<InequalityExpression>(std::move(lc), std::move(rc));
            } else {
                return std::make_shared<EqualityExpression>(std::move(lc), std::move(rc));
            }
        } else if (strcmp(element->name(), "neq") == 0 || strcmp(element->name(), "inequality") == 0) {
            if (notFlag) {
                return std::make_shared<EqualityExpression>(std::move(lc), std::move(rc));
            } else {
                return std::make_shared<InequalityExpression>(std::move(lc), std::move(rc));
            }
        }
    }

    throw base_error("Could not parse '", element->name(), "' as a guard expression");
    return nullptr;
}

std::vector<ColorExpression_ptr> PNMLParser::parseColorExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "dotconstant") == 0) {
        return {std::make_shared<DotConstantExpression>()};
    } else if (strcmp(element->name(), "variable") == 0) {
        return {std::make_shared<VariableExpression>(variables[element->first_attribute("refvariable")->value()])};
    } else if (strcmp(element->name(), "useroperator") == 0) {
        return {std::make_shared<UserOperatorExpression>(findColor(element->first_attribute("declaration")->value()))};
    } else if (strcmp(element->name(), "successor") == 0) {
        auto expr = parseColorExpression(element->first_node());
        for(auto& e : expr)
            e = std::make_shared<SuccessorExpression>(std::move(e));
        return expr;
    } else if (strcmp(element->name(), "predecessor") == 0) {
        auto expr = parseColorExpression(element->first_node());
        for(auto& e : expr)
            e = std::make_shared<PredecessorExpression>(std::move(e));
        return expr;
    } else if (strcmp(element->name(), "finiteintrangeconstant") == 0) {
        auto value = element->first_attribute("value")->value();
        auto intRangeElement = element->first_node("finiteintrange");
        const char* start = intRangeElement->first_attribute("start")->value();
        const char* end = intRangeElement->first_attribute("end")->value();
        return {std::make_shared<UserOperatorExpression>(findColorForIntRange(value, start, end))};

    } else if (strcmp(element->name(), "tuple") == 0) {
        std::vector<std::vector<ColorExpression_ptr>> products;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            products.emplace_back(parseColorExpression(it));
        }
        std::vector<ColorExpression_ptr> result;
        std::vector<size_t> indexes(products.size(), 0);

        do {
            size_t li = 0;
            std::vector<ColorExpression_ptr> constituents;
            for(size_t i = 0; i < indexes.size(); ++i)
            {
                constituents.push_back(products[i][indexes[i]]);
                if(indexes[i] + 1 < products[i].size())
                    li = i;
            }
            result.emplace_back(std::make_shared<TupleExpression>(std::move(constituents), colorTypes));
            indexes[li] += 1;
            std::fill(indexes.begin() + li + 1, indexes.end(), 0);
        } while(indexes[0] < products[0].size());

        return result;
    } else if (strcmp(element->name(), "subterm") == 0 || strcmp(element->name(), "structure") == 0) {
        return parseColorExpression(element->first_node());
    } else if (strcmp(element->name(), "all") == 0) {
        auto* sort = parseUserSort(element);
        std::vector<ColorExpression_ptr> colors;
        for(auto& c : *sort)
            colors.emplace_back(std::make_shared<UserOperatorExpression>(&c));
        return colors;
    }
    throw base_error("Unhandled color type");
    return {};
}

AllExpression_ptr PNMLParser::parseAllExpression(rapidxml::xml_node<>* element) {
    if (strcmp(element->name(), "all") == 0) {
        return std::make_shared<AllExpression>(parseUserSort(element));
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parseAllExpression(element->first_node());
    }

    return nullptr;
}

const ColorType* PNMLParser::parseUserSort(rapidxml::xml_node<>* element) {
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

ArcExpression_ptr PNMLParser::parseNumberOfExpression(rapidxml::xml_node<>* element) {
    auto num = element->first_node();
    uint32_t number = parseNumberConstant(num);
    rapidxml::xml_node<>* first;
    if (number) {
        first = num->next_sibling();
    } else {
        number = 1;
        first = num;
    }
    
    std::vector<std::vector<ColorExpression_ptr>> constituents;
    for (auto it = first; it; it = it->next_sibling()) {
        constituents.emplace_back(parseColorExpression(it));
    }
    // do all possible subsets
    std::vector<ArcExpression_ptr> result;
    std::vector<size_t> indexes(constituents.size(), 0);
    do {
        std::vector<ColorExpression_ptr> elements;
        size_t li = 0;
        for(size_t i = 0; i < constituents.size(); ++i)
        {
            elements.emplace_back(constituents[i][indexes[i]]);
            if(indexes[i] + 1 < constituents[i].size()) // remember last index that can increment
                li = i;
        }
        result.emplace_back(std::make_shared<NumberOfExpression>(std::move(elements), number));

        ++indexes[li];
        std::fill(indexes.begin() + li + 1, indexes.end(), 0);
    } while(indexes[0] < constituents[0].size());
    if(result.size() == 1)
        return result[0];
    else
        return std::make_shared<AddExpression>(std::move(result));
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
            throw base_error("variable not supported");
        } else if (strcmp(it->name(),"queries") == 0) {
            parseQueries(it);
        } else if (strcmp(it->name(), "k-bound") == 0) {
            throw base_error("k-bound should be given as command line option -k");
        } else if (strcmp(it->name(),"query") == 0) {
            throw base_error("query tag not supported, please use PQL or XML-style queries instead");
        }
        else
        {
            parseElement(it);
        }
    }
}

void PNMLParser::parseQueries(rapidxml::xml_node<>* element) {
    std::string query;

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        std::string name(element->first_attribute("name")->value());
        parseValue(element, query);
        Query q;
        q.name = name;
        q.text = query;
        this->queries.push_back(q);
    }
}

void PNMLParser::parsePlace(rapidxml::xml_node<>* element) {
    double x = 0, y = 0;
    std::string id(element->first_attribute("id")->value());

    auto initial = element->first_attribute("initialMarking");
    uint64_t initialMarking = 0;
    Multiset hlinitialMarking;
    const ColorType* type = nullptr;
    if(initial)
         initialMarking = atoll(initial->value());

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parsePosition(it, x, y);
        } else if (strcmp(it->name(),"initialMarking") == 0) {
            std::string text;
            parseValue(it, text);
            initialMarking = atoll(text.c_str());
        } else if (strcmp(it->name(),"hlinitialMarking") == 0) {
            std::unordered_map<const Variable*, const Color*> binding;
            EquivalenceVec placePartition;
			ExpressionContext context {binding, colorTypes, placePartition};
            auto ae = parseArcExpression(it->first_node("structure"));
            hlinitialMarking = EvaluationVisitor::evaluate(*ae, context);
        } else if (strcmp(it->name(), "type") == 0) {
            type = parseUserSort(it);
            placeTypeContext = type->getName();

        }
    }

    if(initialMarking > std::numeric_limits<uint32_t>::max())
    {
        throw base_error("Number of tokens in ", id, " exceeded ", std::numeric_limits<uint32_t>::max());
    }
    //Create place
    if (!isColored) {
        builder->addPlace(id, initialMarking, x, y);
    } else {
        if (!type) {
            throw base_error("Place '", id, "' is missing color type");
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
    placeTypeContext = "";
}

void PNMLParser::parseArc(rapidxml::xml_node<>* element, bool inhibitor) {
    std::string source = element->first_attribute("source")->value(),
            target = element->first_attribute("target")->value();
    int weight = 1;
    auto type = element->first_attribute("type");
    if(type && strcmp(type->value(), "timed") == 0)
    {
        throw base_error("timed arcs are not supported");
    }
    else if(type && strcmp(type->value(), "inhibitor") == 0)
    {
        inhibitor = true;
    }

    bool first = true;
    auto weightTag = element->first_attribute("weight");
    if(weightTag != nullptr) {
        weight = atoi(weightTag->value());
        assert(weight > 0);
    } else if (isColored && inhibitor) {
        auto hlin = element->first_node("hlinscription");
        if (hlin != nullptr) {
            auto structure = hlin->first_node("structure");
            if (structure != nullptr) {
                auto numberOf = structure->first_node("numberof");
                if (numberOf != nullptr) {
                    for (auto subterm = numberOf->first_node("subterm"); subterm; subterm = subterm->next_sibling("subterm")) {
                        auto numberConstant = subterm->first_node("numberconstant");
                        if (numberConstant != nullptr) {
                            auto valueTag = numberConstant->first_attribute("value");
                            weight = atoi(valueTag->value());
                        }
                    }
                }
            }
        }
    } else {
        for (auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")) {
            std::string text;
            parseValue(it, text);
            weight = atoi(text.c_str());
            if(std::find_if(text.begin(), text.end(), [](char c) { return !std::isdigit(c) && !std::isblank(c); }) != text.end())
            {
                throw base_error("Found non-integer-text in inscription-tag (weight) on arc from ", source, " to ", target, " with value \"", text, "\". An integer was expected.");
            }
            assert(weight > 0);
            if(!first)
            {
                throw base_error("Multiple inscription tags in xml of a arc from ", source, " to ", target, ".");
            }
            first = false;
        }
    }

    ArcExpression_ptr expr;
    first = true;
    for (auto it = element->first_node("hlinscription"); it; it = it->next_sibling("hlinscription")) {
        expr = parseArcExpression(it->first_node("structure"));
        if(!first)
        {
            throw base_error("Multiple hlinscription tags in xml of a arc from ", source, " to ", target, ".");
        }
        first = false;
    }

    if (isColored && !inhibitor)
        assert(expr != nullptr);
    Arc arc;
    arc.source = source;
    arc.target = target;
    arc.weight = weight;
    arc.inhib = inhibitor;
    if(!inhibitor)
        arc.expr = expr;
    assert(weight > 0);

    if(weight != 0)
    {
        arcs.push_back(arc);
    }
    else
    {
        throw base_error("Arc from ", source, " to ", target, " has non-sensible weight 0.");
    }
}

void PNMLParser::parseTransportArc(rapidxml::xml_node<>* element){
    std::string source	= element->first_attribute("source")->value(),
           transiton	= element->first_attribute("transition")->value(),
           target	= element->first_attribute("target")->value();
    int weight = 1;

    for(auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")){
        std::string text;
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
            t.expr = parseGuardExpression(it->first_node("structure"), false);
        } else if (strcmp(it->name(), "conditions") == 0) {
            throw base_error("conditions not supported");
        } else if (strcmp(it->name(), "assignments") == 0) {
            throw base_error("assignments not supported");
        }
    }


    if(auto pl_el = element->first_node("player")) {
        std::string out;
        parseValue(pl_el, out);
        t._player = atoi(out.c_str());
    }
    else if(auto pl_el = element->first_attribute("player"))
    {
        t._player = atoi(pl_el->value());
    }


    //Add transition to list
    _transitions.push_back(t);
    //Map id to name
    NodeName nn;
    nn.id = t.id;
    nn.isPlace = false;
    id2name[t.id] = nn;
}

void PNMLParser::parseValue(rapidxml::xml_node<>* element, std::string& text) {
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

const Color* PNMLParser::findColor(const char* name) const {
    bool placeTypeIsTuple = false;
    if (!hasPartition && !placeTypeContext.empty()) {
        auto &placeColorType = colorTypes.find(placeTypeContext)->second;
        placeTypeIsTuple = placeColorType->productSize() > 1;
    }

    for (const auto& elem : colorTypes) {
        if (!hasPartition && !placeTypeContext.empty()) {
            //If we are finding color for a place, and the type of place is tuple
            if (placeTypeIsTuple) {
                auto &placeColorType = colorTypes.find(placeTypeContext)->second;
                const auto *pt = dynamic_cast<const ProductType *> (placeColorType);
                //go through all colotypes in the tuple
                for (uint32_t i = 0; i < placeColorType->productSize(); i++) {
                    const auto &nestedColorType = pt->getNestedColorType(i);
                    if (nestedColorType->productSize() > 1) {
                        throw base_error("Nested products when finding colors: ", nestedColorType->getName());
                    }
                    auto col = (*nestedColorType)[name];
                    if (col) {
                        return col;
                    }
                }
                //type of place is not a tuple
            } else if (elem.first != placeTypeContext) {
                continue;
            }
        }
        auto col = (*elem.second)[name];
        if (col)
            return col;
    }
    throw base_error("Could not find color: ", name, "\nCANNOT_COMPUTE\n");
}

std::vector<ColorExpression_ptr> PNMLParser::findPartitionColors(rapidxml::xml_node<>* element) const {
    std::vector<ColorExpression_ptr> colorExpressions;
    char *name;
    if (strcmp(element->name(), "useroperator") == 0) {
         name = element->first_attribute("declaration")->value();
    } else if (strcmp(element->name(), "successor") == 0) {
        auto colorExpressionVec = findPartitionColors(element->first_node());
        for(auto &colorExpression : colorExpressionVec){
            colorExpressions.push_back(std::make_shared<SuccessorExpression>(std::move(colorExpression)));
        }
        return colorExpressions;
    } else if (strcmp(element->name(), "predecessor") == 0) {
        auto colorExpressionVec = findPartitionColors(element->first_node());
        for(auto &colorExpression : colorExpressionVec){
            colorExpressions.push_back(std::make_shared<PredecessorExpression>(std::move(colorExpression)));
        }
        return colorExpressions;
    } else if (strcmp(element->name(), "variable") == 0 || strcmp(element->name(), "dotconstant") == 0 || strcmp(element->name(), "finiteintrangeconstant") == 0) {
        return colorExpressions;
    } else if (strcmp(element->name(), "subterm") == 0) {
        return findPartitionColors(element->first_node());
    } else {
        throw base_error("Could not find color expression in expression: ", element->name(), "\nCANNOT_COMPUTE\n");
    }
    for (auto partition : partitions) {
        if (strcmp(partition.name.c_str(), name) == 0){
            for(auto color : partition.colors){

                colorExpressions.push_back(std::make_shared<UserOperatorExpression>(color));

            }
        }
    }
    return colorExpressions;
}

const Color* PNMLParser::findColorForIntRange(const char* value, const char* start, const char* end) const {
    bool placeTypeIsTuple = false;
    if (!hasPartition && !placeTypeContext.empty()) {
        auto &placeColorType = colorTypes.find(placeTypeContext)->second;
        placeTypeIsTuple = placeColorType->productSize() > 1;
    }


    for (const auto& elem : colorTypes) {
        if (!hasPartition && !placeTypeContext.empty()) {
            if (placeTypeIsTuple) {
                auto &placeColorType = colorTypes.find(placeTypeContext)->second;
                const auto *pt = dynamic_cast<const ProductType *> (placeColorType);
                //go through all colotypes in the tuple
                for (uint32_t i = 0; i < placeColorType->productSize(); i++) {
                    const auto &nestedColorType = pt->getNestedColorType(i);
                    if (nestedColorType->isProduct()) {
                        throw base_error("Nested products when finding colors: ", nestedColorType->getName());
                    }
                    const std::string& fn = (*nestedColorType)[size_t{0}].getColorName();
                    const std::string& ln = (*nestedColorType)[(*nestedColorType).size() - 1].getColorName();
                    if (fn.compare(start) == 0 && ln.compare(end) == 0) {
                        return &(*nestedColorType)[atoll(value) - atoll(start)];
                    }
                }
                //type of place is not a tuple
            } else if (elem.first != placeTypeContext) {
                continue;
            }
        }
        if (elem.second->isProduct()) continue;
        const std::string& fn = (*elem.second)[size_t{0}].getColorName();
        const std::string& ln = (*elem.second)[(*elem.second).size() - 1].getColorName();
        if (fn.compare(start) == 0 && ln.compare(end) == 0) {
            return &(*elem.second)[atoll(value) - atoll(start)];
        }
    }
    throw base_error("Could not find color: ", value, "\nCANNOT_COMPUTE\n");
}
