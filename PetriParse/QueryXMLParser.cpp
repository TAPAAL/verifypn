#include "QueryXMLParser.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace XMLSP;
using namespace std;

bool QueryXMLParser::parse(const std::string& xml){
	//Parser the xml
	DOMElement* root = DOMElement::loadXML(xml);
	parsePropertySet(root);

	//Release DOM tree
	delete root;
	return true;
}


void QueryXMLParser::parsePropertySet(DOMElement* element){
	if (element->getElementName()!="property-set") {
		cout << "ERROR missing property-set" << endl;
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
	}
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
	//	cout << (*it)->getElementName() << endl;
		if ((*it)->getElementName()=="id") {
			cout << "ID: " << (*it)->getCData() << endl;
		} else if ((*it)->getElementName()=="formula") {
			parseFormula(*it);
		} else if ((*it)->getElementName()=="tags") {
			parseTags(*it);
		}
	}
	cout << endl;
}

void QueryXMLParser::parseTags(DOMElement* element){
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
		cout << (*it)->getElementName() << endl;
	}
	cout << endl;
}

void QueryXMLParser::parseFormula(DOMElement* element){
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
	//	cout << (*it)->getElementName() << endl;
	}
	cout << endl;
}

