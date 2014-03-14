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
	try {
	parsePropertySet(root);
	} catch (int e) {
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
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	if (elements.size() != 1) {
		return false;
	}
	string elementName = (*elements.begin())->getElementName(); 
	if (elementName=="invariant") {
		queryText="INV";
	} else if (elementName=="impossibility") {
		queryText="IMPOSIB";
	} else if (elementName=="possibility") {
		queryText="POSIB";
	} else {
		return false;
	}
	
	return true;
}

