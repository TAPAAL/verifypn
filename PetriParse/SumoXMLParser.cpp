#include <iostream>
#include "xmlsp/xmlsp.h"
#include "SumoXMLParser.h"

using namespace std;


bool SumoXMLParser::on_tag_open(const std::string& tag_name, XMLSP::StringMap& attributes)
{
	XMLSP::StringMap::iterator i;
	cout<<"<"<<tag_name;

	for(i = attributes.begin(); i != attributes.end(); i++) {
		cout<<" "<<i->first<<"=\""<<i->second<<"\"";
	}

	cout<<">\n";

	return true;
}

bool SumoXMLParser::on_cdata(const std::string& cdata)
{
	cout<<"CDATA: "<<cdata<<endl;
	return true;
}

bool SumoXMLParser::on_tag_close(const std::string& tag_name)
{
	cout<<"</"<<tag_name<<">"<<endl;
	return true;
}

bool SumoXMLParser::on_comment(const std::string& comment)
{
	cout<<"<!--"<<comment<<"-->"<<endl;
	return true;
}

bool SumoXMLParser::on_processing(const std::string& value)
{
	cout<<"<?"<<value<<"?>"<<endl;
	return true;
}

bool SumoXMLParser::on_doctype(const std::string& value)
{
	cout<<"<!"<<value<<">"<<endl;
	return true;
}

bool SumoXMLParser::on_document_begin()
{
	cout<<"DOCUMENT BEGIN"<<endl;
	return true;
}

bool SumoXMLParser::on_document_end()
{
	cout<<"DOCUMENT END"<<endl;
	return true;
}

void SumoXMLParser::on_error(int errnr, int line, int col, const std::string& message)
{
	cout<<"ERROR("<<errnr<<"): "<<message<<", at "<<line<<":"<<col<<endl;
}





