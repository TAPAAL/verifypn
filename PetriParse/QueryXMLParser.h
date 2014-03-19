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
#include <string.h>
#include <vector>
#include <sstream> 

using namespace std;

class QueryXMLParser {
public:    
	QueryXMLParser();
 //      ~QueryXMLParser();
    
        struct QueryItem {
            string id; // query name
            string queryText; // only EF queries will be here
            bool negateResult; // true if the final result should be negated
            bool isPlaceBound; // true if the query is a place-bound one (returns integer)
            enum { 
                PARSING_OK,
                UNSUPPORTED_QUERY,
            } parsingResult;
        };

        typedef vector<QueryItem> Queries;
        typedef Queries::iterator QueriesIterator;
        Queries queries;

	bool parse(const string& xml);
        void printQueries();
        
private:
        void parsePropertySet(XMLSP::DOMElement* element);
        void parseProperty(XMLSP::DOMElement* element);
        bool parseTags(XMLSP::DOMElement* element);
        bool parseFormula(XMLSP::DOMElement* element, string &queryText, bool &negateResult, bool &isPlaceBound);
        bool parseBooleanFormula(XMLSP::DOMElement* element, string &queryText);
        bool parseIntegerExpression(XMLSP::DOMElement* element, string &queryText);
        
        enum {MISSING_PROPERTY_SET, MISSING_PROPERTY, EMPTY_QUERY_ID, EXPECTED_INTEGER};
        
};

#endif	/* QUERYXMLPARSER_H */

