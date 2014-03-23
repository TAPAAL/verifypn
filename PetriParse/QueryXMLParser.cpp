#include "QueryXMLParser.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream> 

using namespace XMLSP;
using namespace std;

QueryXMLParser::QueryXMLParser() { };

// QueryXMLParser::~QueryXMLParser() { };


bool QueryXMLParser::parse(const std::string& xml){
	//Parser the xml
	DOMElement* root = DOMElement::loadXML(xml);
	bool parsingOK=true;
	if (root) {
		try {
			parsePropertySet(root);
		} catch (ERRORS e) {
			parsingOK=false;
		}
	} else {
		parsingOK=false;
	}
		
	//Release DOM tree
	delete root;
	return parsingOK;
}


void QueryXMLParser::parsePropertySet(DOMElement* element){
	if (element->getElementName()!="property-set") {
		cout << "ERROR missing property-set" << endl;
		throw MISSING_PROPERTY_SET; // missing property-set element
	}
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
		parseProperty(*it);
	}
}

void QueryXMLParser::parseProperty(DOMElement* element){
	if (element->getElementName()!="property") {
		cout << "ERROR missing property" << endl;
		throw MISSING_PROPERTY; // unexpected element (only property is allowed)
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
	//	cout << (*it)->getElementName() << endl;
		if ((*it)->getElementName()=="id") {
			id = (*it)->getCData();
			//cout << "ID: " << (*it)->getCData() << endl;
		} else if ((*it)->getElementName()=="formula") {
			formulaPtr=it;
		} else if ((*it)->getElementName()=="tags") {
			tagsOK=parseTags(*it);
		}
	}
	
	if (id=="") {
		throw EMPTY_QUERY_ID;
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
		//cout << "parseBooleanFormula: " << element->getElementName() << endl;
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
			queryText+= "not("+subformula1+") or ( "+subformula2;
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
		}
	return false;
}

bool QueryXMLParser::parseIntegerExpression(DOMElement* element, string &queryText){
	string elementName = element->getElementName();
	if (elementName == "integer-constant") {
		int i;
		if(sscanf((element->getCData()).c_str(), "%d", &i)  == EOF ) { 
			throw EXPECTED_INTEGER;
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
			queryText+="\""+parsePlace(children[i])+"\"";
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
		throw NOT_A_PLACE;
	}
	string placeName = element->getCData();
	placeName.erase(std::remove_if(placeName.begin(), placeName.end(), ::isspace), placeName.end());
	return placeName;
}
 

void QueryXMLParser::printQueries() {
	QueryXMLParser::QueriesIterator it;
	for(it = queries.begin(); it != queries.end(); it++){
			cout << it->id << ": " << (it->isPlaceBound ? "\tplace-bound " : "");
			if (it->parsingResult == QueryItem::UNSUPPORTED_QUERY) {
				cout << "\t---------- unsupported query ----------" << endl;
			} else {
			cout << "\t" << (it->negateResult ? "not " : "") << it->queryText << endl;
			}
	}	
}