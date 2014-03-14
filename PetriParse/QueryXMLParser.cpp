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
	DOMElements booleanFormula = (*elements.begin())->getChilds();
	string elementName = (*elements.begin())->getElementName(); 
	if (elementName=="invariant") {
		queryText="INV ( ";
	} else if (elementName=="impossibility") {
		queryText="IMPOSIB ( ";
	} else if (elementName=="possibility") {
		queryText="POSIB ( ";
	} else if (elementName=="negation") {
		DOMElements children = (*elements.begin())->getChilds();
		if (children.size() !=1) {
			return false;
		}
		booleanFormula = (*children.begin())->getChilds();
		string negElementName = (*children.begin())->getElementName();
		if (negElementName=="invariant") {
			queryText="NEG INV ( ";
		} else if (negElementName=="impossibility") {
			queryText="NEG IMPOSIB ( ";
		} else if (negElementName=="possibility") {
			queryText="NEG POSIB ( ";
		} else {
			return false;
		}
	} else {
		return false;
	}
	if (!parseBooleanFormula(*booleanFormula.begin(), queryText)) {
		return false;
	}
	queryText+=" )";
	return true;
}

bool QueryXMLParser::parseBooleanFormula(DOMElement* element, string &queryText){
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
	//	cout << (*it)->getElementName() << endl;
		string elementName = (*it)->getElementName();
		if (elementName=="true") {
			queryText+="TRUE";
			return true;
		} else if (elementName=="false") {
			queryText+="FALSE";
			return true;
		} else if (elementName=="negation") {
			DOMElements children = (*it)->getChilds();
			queryText+="NOT (";
			if (children.size()==1 && parseBooleanFormula(*children.begin(), queryText)) {
				queryText+=" )";
			} else {
				return false;
			}
			return true;
		} else if (elementName=="conjunction") {
			DOMElements children = (*it)->getChilds();
			if (children.size()<2) {
				return false;
			}
			if (!(parseBooleanFormula(*children.begin(), queryText))) {
				return false;
			}
			DOMElements::iterator it;
			for(it = elements.begin()+1; it != elements.end(); it++) {
				queryText+=" AND ";
				if (!(parseBooleanFormula(*it, queryText))) {
					return false;
				}
			}
			return true;
		} else if (elementName=="disjunction") {
			DOMElements children = (*it)->getChilds();
			if (children.size()<2) {
				return false;
			}
			if (!(parseBooleanFormula(*children.begin(), queryText))) {
				return false;
			}
			DOMElements::iterator it;
			for(it = elements.begin()+1; it != elements.end(); it++) {
				queryText+=" OR ";
				if (!(parseBooleanFormula(*it, queryText))) {
					return false;
				}
			}
			return true;
		} else if (elementName=="exclusive-disjunction") {
			DOMElements children = (*it)->getChilds();
			if (children.size()!=2) { // we support only two subformulae here
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*children.begin(), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "( "+subformula1+" AND NOT ( "+subformula2+" ) ) OR ( NOT ( "+subformula1+" ) AND "+subformula2+"  )";
			return true;
		} else if (elementName=="implication") {
			DOMElements children = (*it)->getChilds();
			if (children.size()!=2) { // implication has only two subformulae
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*children.begin(), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "( NOT ( "+subformula1+" ) OR ( "+subformula2+" )";
			return true;
		} else if (elementName=="equivalence") {
			DOMElements children = (*it)->getChilds();
			if (children.size()!=2) { // we support only two subformulae here
				return false;
			}
			string subformula1;
			string subformula2;
			if (!(parseBooleanFormula(*children.begin(), subformula1))) {
				return false;
			}
			if (!(parseBooleanFormula(*(children.begin()+1), subformula2))) {
				return false;
			}
			queryText+= "( "+subformula1+" AND "+subformula2+" ) OR ( NOT ( "+subformula1+" ) AND NOT ( "+subformula2+" ) )";
			return true;
		}
	}
	return true;
}

