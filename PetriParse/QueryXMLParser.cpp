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
	parseElement(root);

	//Release DOM tree
	delete root;
	return true;
}

void QueryXMLParser::parseElement(DOMElement* element){
	cout << (element)->getElementName() << endl;
	DOMElements elements = element->getChilds();
	DOMElements::iterator it;
	for(it = elements.begin(); it != elements.end(); it++){
		if((*it)->getElementName() == "id"){
			cout << (*it)->getCData() << endl;
			parseElement(*it);
		} else {
			parseElement(*it);
		}	
	}
}

