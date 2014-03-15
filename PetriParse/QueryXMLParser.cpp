#include "QueryXMLParser.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

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
		} catch (int e) {
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
	bool negateResult;
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
	if (tagsOK && parseFormula(*formulaPtr, queryText, negateResult)) {
		queryItem.queryText=queryText;
		queryItem.negateResult=negateResult;
		queryItem.parsingResult=QueryItem::PARSING_OK;
	} else {
		queryItem.queryText="";
		queryItem.negateResult=false;
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

bool QueryXMLParser::parseFormula(DOMElement* element, string &queryText, bool &negateResult){
	/*
	 Describe here how to parse
	 * INV phi
	 * IMPOS phi
	 * POS phi
	 * NEG INV phi
	 * NEG IMPOS phi
	 * NEG POS phi
	 */
	DOMElements elements = element->getChilds();
	if (elements.size() != 1) {
		return false;
	}
	DOMElements::iterator booleanFormula = elements.begin();
	string elementName = (*booleanFormula)->getElementName(); 
	if (elementName=="invariant") {
		queryText="INV (";
	} else if (elementName=="impossibility") {
		queryText="IMPOSIB (";
	} else if (elementName=="possibility") {
		queryText="POSIB (";
	} else if (elementName=="negation") {
		DOMElements children = (*elements.begin())->getChilds();
		if (children.size() !=1) {
			return false;
		}
		booleanFormula = children.begin(); 
		string negElementName = (*booleanFormula)->getElementName();
		if (negElementName=="invariant") {
			queryText="NEG INV (";
		} else if (negElementName=="impossibility") {
			queryText="NEG IMPOSIB (";
		} else if (negElementName=="possibility") {
			queryText="NEG POSIB (";
		} else {
			return false;
		}
	} else {
		return false;
	}
	DOMElements nextElements = (*booleanFormula)->getChilds();
	if (nextElements.size() !=1 || !parseBooleanFormula(nextElements[0] , queryText)) {
		return false;
	}
	queryText+=")";
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
			queryText+= "( ( "+subformula1+" and not( "+subformula2+" )) or ( not( "+subformula1+" ) and "+subformula2+"  ) )";
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
			queryText+= "( ( "+subformula1+" and "+subformula2+" ) or ( not ( "+subformula1+" ) and not ( "+subformula2+" ) ) )";
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
		queryText+=element->getCData();
		return true;
	} else if (elementName == "tokens-count") {
		DOMElements children = element->getChilds();
		if (children.size()==0) {
			return false;
		}
		for (int i = 0; i < children.size(); i++) {
			if (children[i]->getElementName() != "place") {
				return false;
			}
			if (i > 0) {
				queryText += " + ";
			}
			queryText+=children[i]->getCData();
		}
		return true;
	}
	return false;
}