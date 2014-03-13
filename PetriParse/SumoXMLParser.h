/* 
 * File:   SumoXMLParser.h
 * Author: srba
 *
 * Created on March 13, 2014, 3:43 PM
 */

#ifndef SUMOXMLPARSER_H
#define	SUMOXMLPARSER_H


#include <iostream>
#include "xmlsp/xmlsp.h"

using namespace std;

class SumoXMLParser: public XMLSP::Parser
{
public:

	virtual bool on_tag_open(const std::string& tag_name, XMLSP::StringMap& attributes);
	virtual bool on_cdata(const std::string& cdata);
	virtual bool on_tag_close(const std::string& tag_name);
	virtual bool on_comment(const std::string& comment);
	virtual bool on_processing(const std::string& value);
	virtual bool on_doctype(const std::string& value);
	virtual bool on_document_begin();
	virtual bool on_document_end();
	virtual void on_error(int errnr, int line, int col, const std::string& message);

};

#endif	/* SUMOXMLPARSER_H */

