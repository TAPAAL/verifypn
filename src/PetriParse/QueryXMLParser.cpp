/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2014 Jiri Srba <srba.jiri@gmail.com>,
 *                    Peter Gjøl Jensen <root@petergjoel.dk>
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

#include "PetriParse/QueryXMLParser.h"
#include "PetriEngine/PQL/Expressions.h"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>

using namespace std;

int getChildCount(rapidxml::xml_node<> *n)
{
  int c = 0;
  for (rapidxml::xml_node<> *child = n->first_node(); child != nullptr; child = child->next_sibling())
  {
    c++;
  } 
  return c;
} 

QueryXMLParser::QueryXMLParser() = default;

QueryXMLParser::~QueryXMLParser() = default;

bool QueryXMLParser::parse(std::ifstream& xml, const std::set<size_t>& parse_only) {
    //Parse the xml
    rapidxml::xml_document<> doc;
    vector<char> buffer((istreambuf_iterator<char>(xml)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    rapidxml::xml_node<>*  root = doc.first_node();
    bool parsingOK;
    if (root) {
        parsingOK = parsePropertySet(root, parse_only);
    } else {
        parsingOK = false;
    }

    //Release DOM tree
    return parsingOK;
}

bool QueryXMLParser::parsePropertySet(rapidxml::xml_node<>*  element, const std::set<size_t>& parse_only) {
    if (strcmp(element->name(), "property-set") != 0) {
        fprintf(stderr, "ERROR missing property-set\n");
        return false; // missing property-set element
    }
    
    size_t i = 0;
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if(parse_only.empty() || parse_only.count(i) > 0)
        {
            if (!parseProperty(it)) {
                return false;
            }
        }
        else
        {
            QueryItem queryItem;
            queryItem.query = nullptr;
            queryItem.parsingResult = QueryItem::PARSING_OK;
            queries.push_back(queryItem);
        }
        ++i;
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
    rapidxml::xml_node<>* formulaPtr = nullptr;
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "id") == 0) {
            id = it->value();
        } else if (strcmp(it->name(), "formula") == 0) {
            formulaPtr = it;
        } else if (strcmp(it->name(), "tags") == 0) {
            tagsOK = parseTags(it);
        }
    }

    if (id.empty()) {
        fprintf(stderr, "ERROR a query with empty id\n");
        return false;
    }

    QueryItem queryItem;
    queryItem.id = id;
    if(tagsOK)
    {
        queryItem.query = parseFormula(formulaPtr);
        assert(queryItem.query);
        queryItem.parsingResult = QueryItem::PARSING_OK;
    } else {
        queryItem.query = nullptr;
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

void QueryXMLParser::fatal_error(const std::string &token) {
    std::cerr << "An error occurred while parsing the query." << std::endl;
    std::cerr << token << std::endl;
    assert(false);
    exit(ErrorCode);
}

Condition_ptr QueryXMLParser::parseFormula(rapidxml::xml_node<>*  element) {
    if (getChildCount(element) != 1) 
    {
        assert(false);
        return nullptr;    
    }
    auto child = element->first_node();
    string childName = child->name();
    Condition_ptr cond = nullptr;
    
    // Formula is either CTL/Reachability, UpperBounds or one of the global properties
    // - k-safe (contains integer bound) : for all p. it holds that AG p <= bound
    // - quasi-liveness : for all t. EF is-fireable(t)
    // - stable-marking : exists p. and x. s.t. AG p == x (i.e. the initial marking)
    // - liveness : for all t. AG EF is-fireable(t)
    // we unfold global properties here to avoid introducing special-cases in the AST.
    if (childName == "k-safe")
    {
        Expr_ptr bound = nullptr;
        for (auto it = child->first_node(); it ; it = it->next_sibling()) {
            if(bound != nullptr) fatal_error(childName);
            bound = parseIntegerExpression(child->first_node());
            if(bound == nullptr) fatal_error(childName);
        }
        if(bound == nullptr) fatal_error(childName);
        return std::make_shared<KSafeCondition>(bound);
    }
    else if(childName == "quasi-liveness")
    {
        return std::make_shared<QuasiLivenessCondition>();
    }
    else if(childName == "stable-marking")
    {
        return std::make_shared<StableMarkingCondition>();
    }
    else if(childName == "liveness")
    {
        return std::make_shared<LivenessCondition>();
    }
    else if (childName == "place-bound") {
        std::vector<std::string> places;
        for (auto it = child->first_node(); it ; it = it->next_sibling()) {
            if (strcmp(it->name(), "place") != 0) 
            {
                assert(false);
                return nullptr;
            }
            auto place = parsePlace(it);
            if (place.empty())
            {             
                assert(false);
                return nullptr; // invalid place name
            }
            places.push_back(place);
        }
        auto bnds = std::make_shared<UpperBoundsCondition>(places);
        return std::make_shared<EFCondition>(bnds);
    } else if ((cond = parseBooleanFormula(child)) != nullptr) {
        return cond;
    } else {
        assert(false);
        return nullptr;
    }
}


Condition_ptr QueryXMLParser::parseBooleanFormula(rapidxml::xml_node<>*  element) {
    /*
     Describe here how to parse
     * INV phi =  AG phi =  not EF not phi
     * IMPOS phi = AG not phi = not EF phi
     * POS phi = EF phi
     * NEG INV phi = not AG phi = EF not phi
     * NEG IMPOS phi = not AG not phi = EF phi
     * NEG POS phi = not EF phi
     */
    
    string elementName = element->name();
    Condition_ptr cond = nullptr, cond2 = nullptr;
    
    if (elementName == "invariant") {
        if ((cond = parseBooleanFormula(element->first_node())) != nullptr)
            return std::make_shared<NotCondition>(std::make_shared<EFCondition>(std::make_shared<NotCondition>(cond)));
    } else if (elementName == "impossibility") {
        if ((cond = parseBooleanFormula(element->first_node())) != nullptr)
            return std::make_shared<NotCondition>(std::make_shared<EFCondition>(cond));
    } else if (elementName == "possibility") {
        if ((cond = parseBooleanFormula(element->first_node())) != nullptr)
            return std::make_shared<EFCondition>(cond);
    } else if (elementName == "exists-path") {
        if (getChildCount(element) != 1) 
        {
            assert(false);
            return nullptr;
        }
        auto child = element->first_node();
        if (strcmp(child->name(), "next") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<EXCondition>(cond);
        } else if (strcmp(child->name(), "globally") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<EGCondition>(cond);
        } else if (strcmp(child->name(), "finally") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<EFCondition>(cond);
        } else if (strcmp(child->name(), "until") == 0) {
            if (getChildCount(child) != 2) 
            {
                assert(false);
                return nullptr;
            }
            auto before = child->first_node();
            auto reach = before->next_sibling();
            if (getChildCount(before) != 1 || getChildCount(reach) != 1 ||
                    strcmp(before->name(), "before") != 0 || strcmp(reach->name(), "reach") != 0) 
                {
                    assert(false);
                    return nullptr;
                }
            if ((cond = parseBooleanFormula(before->first_node())) != nullptr) {
                if ((cond2 = parseBooleanFormula(reach->first_node())) != nullptr) {
                    return std::make_shared<EUCondition>(cond, cond2);
                }
            }
        }
    } else if (elementName == "all-paths") {
        if (getChildCount(element) != 1) 
        {
            assert(false);
            return nullptr;
        }
        auto child = element->first_node();
        if (strcmp(child->name(), "next") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<AXCondition>(cond);
        } else if (strcmp(child->name(), "globally") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<AGCondition>(cond);
        } else if (strcmp(child->name(), "finally") == 0) {
            if (getChildCount(child) != 1) 
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr)
                return std::make_shared<AFCondition>(cond);
        } else if (strcmp(child->name(), "until") == 0) {
            if (getChildCount(child) != 2) 
            {
                assert(false);
                return nullptr;
            }
            auto before = child->first_node();
            auto reach = before->next_sibling();
            if (getChildCount(before) != 1 || getChildCount(reach) != 1 ||
                    strcmp(before->name(), "before") != 0 || strcmp(reach->name(), "reach") != 0)
            {
                assert(false);
                return nullptr;
            }
            if ((cond = parseBooleanFormula(before->first_node())) != nullptr){
                if ((cond2 = parseBooleanFormula(reach->first_node())) != nullptr) {
                    return std::make_shared<AUCondition>(cond, cond2);
                }
            }
        }
    } else if (elementName == "deadlock") {
        return std::make_shared<DeadlockCondition>();
    } else if (elementName == "true") {
        return BooleanCondition::TRUE_CONSTANT;
    } else if (elementName == "false") {
        return BooleanCondition::FALSE_CONSTANT;
    } else if (elementName == "negation") {
        if (getChildCount(element) != 1) 
        {
            assert(false);
            return nullptr;
        }
        auto child = element->first_node();
        if (strcmp(child->name(), "invariant") == 0) {
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr) {
                return std::make_shared<EFCondition>(std::make_shared<NotCondition>(cond));
            }
        } else if (strcmp(child->name(), "impossibility") == 0) {
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr) {
                return std::make_shared<EFCondition>(cond);
            }
        } else if (strcmp(child->name(), "possibility") == 0) {
            if ((cond = parseBooleanFormula(child->first_node())) != nullptr) {
                return std::make_shared<NotCondition>(std::make_shared<EFCondition>(cond));
            }
        } else {
            if ((cond = parseBooleanFormula(child)) != nullptr) {
                return std::make_shared<NotCondition>(cond);
            }
        }
    } else if (elementName == "conjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) 
        {
            assert(false);
            return nullptr;
        }
        auto it = children;
        cond = parseBooleanFormula(it);
        // skip a sibling
        for (it = it->next_sibling(); it; it = it->next_sibling()) {
            Condition_ptr child = parseBooleanFormula(it);
            if(child == nullptr || cond == nullptr) 
            {
                assert(false);
                return nullptr;
            }
            cond = std::make_shared<AndCondition>(cond, child);
        }
        return cond;
    } else if (elementName == "disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) < 2) 
        {
            assert(false);
            return nullptr;
        }
        auto it = children;
        cond = parseBooleanFormula(it);
        // skip a sibling
        for (it = it->next_sibling(); it; it = it->next_sibling()) {
            Condition_ptr child = parseBooleanFormula(it);
            if(child == nullptr || cond == nullptr) 
            {
                assert(false);
                return nullptr;
            }
            cond = std::make_shared<OrCondition>(cond, child);
        }
        return cond;
    } else if (elementName == "exclusive-disjunction") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) 
        {
            assert(false);
            return nullptr;
        }
        cond = parseBooleanFormula(children);
        cond2 = parseBooleanFormula(children->next_sibling());
        if (cond == nullptr || cond2 == nullptr)    
        {
            assert(false);
            return nullptr;
        }
        return std::make_shared<OrCondition>(
                std::make_shared<AndCondition>(cond, std::make_shared<NotCondition>(cond2)),
                std::make_shared<AndCondition>(std::make_shared<NotCondition>(cond), cond2));
    } else if (elementName == "implication") {
        auto children = element->first_node();
        if (getChildCount(element) != 2)             
        {
            assert(false);
            return nullptr;
        }
        cond = parseBooleanFormula(children);
        cond2 = parseBooleanFormula(children->next_sibling());
        if (cond == nullptr || cond2 == nullptr)
        {
            assert(false);
            return nullptr;       
        }
        return std::make_shared<OrCondition>(std::make_shared<NotCondition>(cond), cond2);
    } else if (elementName == "equivalence") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) 
        {
            assert(false);
            return nullptr;       
        }
        cond = parseBooleanFormula(children);
        cond2 = parseBooleanFormula(children->next_sibling());
        if (cond == nullptr || cond2 == nullptr) return nullptr;
        return std::make_shared<OrCondition>(std::make_shared<AndCondition>(cond, cond2),
                std::make_shared<AndCondition>(std::make_shared<NotCondition>(cond), 
                std::make_shared<NotCondition>(cond2)));
    } else if (elementName == "integer-eq" || elementName == "integer-ne" ||
            elementName == "integer-lt" || elementName == "integer-le" ||
            elementName == "integer-gt" || elementName == "integer-ge") {
        auto children = element->first_node();
        if (getChildCount(element) != 2) 
        {
            assert(false);
            return nullptr;       
        }
        Expr_ptr expr1 = parseIntegerExpression(children);
        Expr_ptr expr2 = parseIntegerExpression(children->next_sibling());
        if(expr1 == nullptr || expr2 == nullptr) 
        {
            assert(false);
            return nullptr;       
        }
        if (elementName == "integer-eq") return std::make_shared<EqualCondition>(expr1, expr2);
        else if (elementName == "integer-ne") return std::make_shared<NotEqualCondition>(expr1, expr2);
        else if (elementName == "integer-lt") return std::make_shared<LessThanCondition>(expr1, expr2);
        else if (elementName == "integer-le") return std::make_shared<LessThanOrEqualCondition>(expr1, expr2);
        else if (elementName == "integer-gt") return std::make_shared<GreaterThanCondition>(expr1, expr2);
        else if (elementName == "integer-ge") return std::make_shared<GreaterThanOrEqualCondition>(expr1, expr2);        
    } else if (elementName == "is-fireable") {
        size_t nrOfChildren = getChildCount(element);
        if (nrOfChildren == 0) 
        {
            assert(false);
            return nullptr;       
        }
        std::vector<Condition_ptr> conds;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            if (strcmp(it->name(), "transition") != 0) 
            {
                assert(false);
                return nullptr;       
            }
            conds.emplace_back(std::make_shared<FireableCondition>(it->value()));
        }
        return std::make_shared<OrCondition>(conds);
    }
    fatal_error(elementName);
    return nullptr;
}

Expr_ptr QueryXMLParser::parseIntegerExpression(rapidxml::xml_node<>*  element) {
    string elementName = element->name();
    if (elementName == "integer-constant") {
        int i;
        if (sscanf(element->value(), "%d", &i) == EOF) 
        {
            assert(false);
            return nullptr;       
        }
        return std::make_shared<LiteralExpr>(i);
    } else if (elementName == "tokens-count") {
        auto children = element->first_node();
        std::vector<Expr_ptr> ids;        
        for (auto it = children; it; it = it->next_sibling()) {
            if (strcmp(it->name(), "place") != 0)
            {
                assert(false);
                return nullptr;
            }
            string placeName = parsePlace(it);
            if (placeName.empty())
            {
                assert(false);
                return nullptr; // invalid place name
            }
            auto id = std::make_shared<IdentifierExpr>(placeName);
            ids.emplace_back(id);
        }
        
        if (ids.empty())
        {
            assert(false);
            return nullptr;       
        }
        if (ids.size() == 1) return ids[0];
        
        return std::make_shared<PlusExpr>(std::move(ids), true);
    } else if (elementName == "integer-sum" || elementName == "integer-product") {
        auto children = element->first_node();
        bool isMult = false;
        if (elementName == "integer-sum") isMult = false;
        else if (elementName == "integer-product") isMult = true;

        std::vector<Expr_ptr> els;
        auto it = children;
       
        for (; it; it = it->next_sibling()) {
            els.emplace_back(parseIntegerExpression(it));
            if(!els.back())  
        {
            assert(false);
            return nullptr;       
        }
        }

        if (els.size() < 2) 
        {
            assert(false);
            return nullptr;       
        }

        return  isMult ? 
                std::dynamic_pointer_cast<Expr>(std::make_shared<MultiplyExpr>(std::move(els))) :
                std::dynamic_pointer_cast<Expr>(std::make_shared<PlusExpr>(std::move(els)));

    } else if (elementName == "integer-difference") {
        auto children = element->first_node();
        std::vector<Expr_ptr> els;
        for (auto it = children; it; it = it->next_sibling()) {
            els.emplace_back(parseIntegerExpression(it));
        }
        if(els.size() == 1) 
            els.emplace(els.begin(), std::make_shared<LiteralExpr>(0));
        return std::make_shared<SubtractExpr>(std::move(els));
    }
    assert(false);
    return nullptr;
}

string QueryXMLParser::parsePlace(rapidxml::xml_node<>*  element) {
    if (strcmp(element->name(), "place") != 0)  return ""; // missing place tag
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
        cout << "\t";
        it.query->toString(cout);
        cout << std::endl;
    }
}

void QueryXMLParser::printQueries() {
    for (size_t i = 1; i <= queries.size(); i++) {
        printQueries(i);
    }
}
