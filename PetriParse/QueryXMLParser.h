/* 
 * File:   QueryXMLParser.h
 * Author: srba
 *
 * Created on March 13, 2014, 3:43 PM
 */

#ifndef QUERYXMLPARSER_H
#define	QUERYXMLPARSER_H

#include "xmlsp/xmlsp_dom.h"

#include <map>
#include <string>
#include <vector>

class QueryXMLParser {
public:    
//	QueryXMLParser();
 //       ~QueryXMLParser();
        
	bool parse(const std::string& xml);
        
private:
        void parsePropertySet(XMLSP::DOMElement* element);
        void parseProperty(XMLSP::DOMElement* element);
        void parseTags(XMLSP::DOMElement* element);
        void parseFormula(XMLSP::DOMElement* element);
};

#endif	/* QUERYXMLPARSER_H */

