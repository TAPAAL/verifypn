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

using namespace XMLSP;
using namespace std;

QueryXMLParser::QueryXMLParser(const PNMLParser::TransitionEnablednessMap &transitionEnabledness) {
	_transitionEnabledness = transitionEnabledness;
};

// QueryXMLParser::~QueryXMLParser() { };


bool QueryXMLParser::parse(const std::string& xml){
	//Parse the xml
	DOMElement* root = DOMElement::loadXML(xml);
	bool parsingOK;
	if (root) {
		parsingOK = parsePropertySet(root);
	} else {
		parsingOK=false;
	}
		
	//Release DOM tree
	delete root;
	return parsingOK;
}


bool QueryXMLParser::parsePropertySet(DOMElement* element){
	if (element->getElementName()!="property-set") {
		fprintf(stderr,"ERROR missing property-set\n");
		return false; // missing property-set element
	}
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
		if (!parseProperty(*it)) {
			return false;
		};
	}
	return true;
}

bool QueryXMLParser::parseProperty(DOMElement* element){
	if (element->getElementName()!="property") {
		fprintf(stderr,"ERROR missing property\n");
		return false; // unexpected element (only property is allowed)
	}
	string id;
	string queryText;
	bool negateResult=false;
    bool isPlaceBound=false;
	string placeNameForBound;
	bool tagsOK=true;
	
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	DOMElements::iterator formulaPtr;
	for(it = elements.begin(); it != elements.end(); it++){
		if ((*it)->getElementName()=="id") {
			id = (*it)->getCData();
		} else if ((*it)->getElementName()=="formula") {
			formulaPtr=it;
		} else if ((*it)->getElementName()=="tags") {
			tagsOK=parseTags(*it);
		}
	}
	
	if (id=="") {
		fprintf(stderr,"ERROR a query with empty id\n");
		return false;
	}
	
	QueryItem queryItem;
	queryItem.id=id;
	if (tagsOK && parseFormula(*formulaPtr, queryText, negateResult, isPlaceBound, placeNameForBound)) {
		queryItem.queryText=queryText;
		queryItem.negateResult=negateResult;
        queryItem.isPlaceBound=isPlaceBound;
		queryItem.placeNameForBound=placeNameForBound;
		queryItem.parsingResult=QueryItem::PARSING_OK;
	} else {
		queryItem.queryText="";
		queryItem.negateResult=false;
        queryItem.isPlaceBound=false;
		queryItem.placeNameForBound="";
		queryItem.parsingResult=QueryItem::UNSUPPORTED_QUERY;
	}	
	queries.push_back(queryItem);
	return true;
}

bool QueryXMLParser::parseTags(DOMElement* element){
	// we can accept only reachability query
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
		if ((*it)->getElementName()=="is-reachability" && (*it)->getCData()!="true") {
			return false;
		}
	}
	return true;
}

bool QueryXMLParser::parseFormula(DOMElement* element, string &queryText, bool &negateResult, bool &isPlaceBound, string &placeNameForBound){
	/*
	 Describe here how to parse
	 * INV phi =  AG phi =  not EF not phi
	 * IMPOS phi = AG not phi = not EF phi
	 * POS phi = EF phi
	 * NEG INV phi = not AG phi = EF not phi
	 * NEG IMPOS phi = not AG not phi = EF phi
	 * NEG POS phi = not EF phi
	 */
	DOMElements elements = element->getChilds();
	if (elements.size() != 1) {
		return false;
	}
	DOMElements::iterator booleanFormula = elements.begin();
	string elementName = (*booleanFormula)->getElementName(); 
	if (elementName=="invariant") {
		queryText="EF not(";
		negateResult=true;
	} else if (elementName=="impossibility") {
		queryText="EF ( ";
		negateResult=true;
	} else if (elementName=="possibility") {
		queryText="EF ( ";
		negateResult=false;
	} else if (elementName=="negation") {
		DOMElements children = (*elements.begin())->getChilds();
		if (children.size() !=1) {
			return false;
		}
		booleanFormula = children.begin(); 
		string negElementName = (*booleanFormula)->getElementName();
		if (negElementName=="invariant") {
			queryText="EF not( ";
			negateResult=false;
		} else if (negElementName=="impossibility") {
			queryText="EF ( ";
			negateResult=false;
		} else if (negElementName=="possibility") {
			queryText="EF ( ";
			negateResult=true;
		} else {
			return false;
		}
    } else if (elementName == "place-bound") {
        queryText = "EF ";
        DOMElements children = (*booleanFormula)->getChilds();
        if (children.size() != 1) {
            return false; // we support only place-bound for one place
        }
        if (children[0]->getElementName() != "place") {
            return false;
        }
        placeNameForBound = parsePlace(children[0]);
		if (placeNameForBound=="") {
			return false; // invalid place name
		}
        queryText += "\""+placeNameForBound+"\""+" < 0";
        negateResult = false;
        isPlaceBound = true;
        return true;
    } else {
            return false;
	}
	DOMElements nextElements = (*booleanFormula)->getChilds();
	if (nextElements.size() !=1 || !parseBooleanFormula(nextElements[0] , queryText)) {
		return false;
	}
	queryText+=" )";
    isPlaceBound=false;
	placeNameForBound = "";
	return true;
}

bool QueryXMLParser::parseBooleanFormula(DOMElement* element, string &queryText){
		string elementName = element->getElementName();
		if (elementName == "deadlock") {
			queryText+="deadlock";
			return true;
		} else if (elementName == "true") {
			queryText+="true";
			return true;
		} else if (elementName == "false") {
			queryText+="false";
			return true;
		} else if (elementName == "negation") {
			DOMElements children = element->getChilds();
			queryText+="not(";
			if (children.size()==1 && parseBooleanFormula(children[0], queryText)) {
				queryText+=")";
			} else {
				return false;
			}
			return true;
		} else if (elementName == "conjunction") {
			DOMElements children = element->getChilds();
			if (children.size()<2) {
				return false;
			}
			if (!(parseBooleanFormula((children[0]), queryText))) {
				return false;
			}
			DOMElements::iterator it;
			for(it = (children.begin())+1; it != children.end(); it++) {
				queryText+=" and ";
				if (!(parseBooleanFormula(*it, queryText))) {
					return false;
				}
			}
			return true;
		} else if (elementName == "disjunction") {
			DOMElements children = element->getChilds();
			if (children.size()<2) {
				return false;
			}
			if (!(parseBooleanFormula(*children.begin(), queryText))) {
				return false;
			}
			DOMElements::iterator it;
			for(it = children.begin()+1; it != children.end(); it++) {
				queryText+=" or ";
				if (!(parseBooleanFormula(*it, queryText))) {
					return false;	
				}
			}
			return true;
		} else if (elementName == "exclusive-disjunction") {
			DOMElements children = element->getChilds();
			if (children.size()!=2) { // we support only two subformulae here
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*(children.begin()), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "(("+subformula1+" and not("+subformula2+")) or (not("+subformula1+") and "+subformula2+"))";
			return true;
		} else if (elementName == "implication") {
			DOMElements children = element->getChilds();
			if (children.size()!=2) { // implication has only two subformulae
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*(children.begin()), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "not("+subformula1+") or ( "+subformula2+" )";
			return true;
		} else if (elementName == "equivalence") {
			DOMElements children = element->getChilds();
			if (children.size()!=2) { // we support only two subformulae here
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*(children.begin()), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "(("+subformula1+" and "+subformula2+") or (not("+subformula1+") and not("+subformula2+")))";
			return true;
		} else if (	elementName == "integer-eq" || 
					elementName == "integer-ne" || 
					elementName == "integer-lt" || 
					elementName == "integer-le" || 
					elementName == "integer-gt" || 
					elementName == "integer-ge") {
			DOMElements children = element->getChilds();
			if (children.size()!=2) { // exactly two integer subformulae are required
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseIntegerExpression(children[0], subformula1))) {
				return false;
			}
			if (!(parseIntegerExpression(children[1], subformula2))) {
				return false;
			}
			string mathoperator;
			if		( elementName == "integer-eq") mathoperator=" == ";
			else if ( elementName == "integer-ne") mathoperator=" != ";
			else if ( elementName == "integer-lt") mathoperator=" < ";
			else if ( elementName == "integer-le") mathoperator=" <= ";
			else if ( elementName == "integer-gt") mathoperator=" > ";
			else if ( elementName == "integer-ge") mathoperator=" >= ";
			
			queryText+="("+subformula1+mathoperator+subformula2+")";
			return true;
		} else if (elementName == "is-fireable") {
			DOMElements children = element->getChilds();
			if (children.size() == 0) {
				return false;
			}
			if (children.size() > 1) {
				queryText += "(";
			}
			for (int i = 0; i < children.size(); i++) {
				if (children[i]->getElementName() != "transition") {
					return false;
				}
				if (i > 0) {
					queryText += " or ";
				}
				string transitionName = children[i]->getCData();
				if(_transitionEnabledness.find(transitionName) == _transitionEnabledness.end()){
					fprintf(stderr,
						"XML Query Parsing error: Transition id=\"%s\" was not found!\n",
					    transitionName.c_str());
					return false;
			    }
				queryText += _transitionEnabledness[transitionName]; 
			}
			if (children.size() > 1) {
				queryText += ")";
			}
			return true;
		}
	return false;
}

bool QueryXMLParser::parseIntegerExpression(DOMElement* element, string &queryText){
	string elementName = element->getElementName();
	if (elementName == "integer-constant") {
		int i;
		if(sscanf((element->getCData()).c_str(), "%d", &i)  == EOF ) { 
			return false; // expected integer at this place
		}
		 stringstream ss;//create a stringstream
         ss << i;//add number to the stream
         queryText+=ss.str();
		return true;
	} else if (elementName == "tokens-count") {
		DOMElements children = element->getChilds();
		if (children.size()==0) {
			return false;
		}
		if (children.size()>1) {
			queryText+="(";
		}
		for (int i = 0; i < children.size(); i++) {
			if (children[i]->getElementName() != "place") {
				return false;
			}
			if (i > 0) {
				queryText += " + ";
			}
			string placeName = parsePlace(children[i]);
			if (placeName == "") {
				return false; // invalid place name
			}
			queryText+="\""+placeName+"\"";
		}
		if (children.size()>1) {
			queryText+=")";
		}
		return true;
	} else if (	elementName == "integer-sum" ||
				elementName == "integer-product" ) {
		DOMElements children = element->getChilds();
		if (children.size() < 2) { // at least two integer subexpression are required
			return false;
		}
		string mathoperator;
		if (	 elementName == "integer-sum") mathoperator = " + ";
		else if (elementName == "integer-product") mathoperator = " * ";
		queryText+="(";
		for (int i = 0; i < children.size(); i++) {
			if (i > 0) {
				queryText+= mathoperator;
			}
			if (!parseIntegerExpression(children[i], queryText)) {
				return false;
			}
		}
		queryText+=")";
		return true;
	} else if (elementName == "integer-difference" ) {
		DOMElements children = element->getChilds();
		if (children.size() != 2) { // at least two integer subexpression are required
			return false;
		}
		queryText+="(";
		if (!parseIntegerExpression(children[0], queryText)) {
			return false;
		}
		queryText+=" - ";
		if (!parseIntegerExpression(children[1], queryText)) {
			return false;
		}
		queryText+=")";
		return true;
	}
	return false;
}

string QueryXMLParser::parsePlace(XMLSP::DOMElement* element) {
	if (element->getElementName() != "place") {
		return ""; // missing place tag
	}
	string placeName = element->getCData();
	placeName.erase(std::remove_if(placeName.begin(), placeName.end(), ::isspace), placeName.end());
	return placeName;
}
 

void QueryXMLParser::printQueries(int i) {
//	QueryXMLParser::QueriesIterator it;
	if (i<=0 || i > queries.size()) {
		cout << "In printQueries the query index is out of scope\n\n";
		return;
	}
	QueryItem it = queries[i-1];
	cout << it.id << ": " << (it.isPlaceBound ? "\tplace-bound " : "");
	if (it.parsingResult == QueryItem::UNSUPPORTED_QUERY) {
		cout << "\t---------- unsupported query ----------" << endl;
	} else {
		cout << "\t" << (it.negateResult ? "not " : "") << it.queryText << endl;
	}
}

void QueryXMLParser::printQueries() {
	for (int i = 1; i <= queries.size(); i++) {
		printQueries(i);
	}	
}
