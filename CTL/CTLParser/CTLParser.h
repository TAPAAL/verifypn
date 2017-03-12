/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLParser_v2.h
 * Author: mossns
 *
 * Created on April 22, 2016, 10:15 AM
 */

#ifndef CTLPARSER_V2_H
#define CTLPARSER_V2_H

#include "CTLQuery.h"

#include "../../PetriParse/rapidxml/rapidxml.hpp"

#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>

struct QueryMeta{
    int numberof_queries = 0;
    std::string model_name = "EmptyString";
};

struct RemoveDelimiter
{
  bool operator()(char c)
  {
    return (c =='\r' || c =='\t' || c == ' ' || c == '\n');
  }
};

class CTLParser {
public:
    CTLParser();
    CTLParser(const CTLParser& orig);
    virtual ~CTLParser();
    CTLQuery * ParseXMLQuery(std::vector<char> buffer, int query_number);
    CTLQuery* FormatQuery(CTLQuery* query, PetriEngine::PetriNet *net);
    std::string QueryToString(CTLQuery* query);
    QueryMeta * GetQueryMetaData(std::vector<char> buffer);
private:
    CTLQuery* xmlToCTLquery(rapidxml::xml_node<> * root);
    std::string parsePar(rapidxml::xml_node<> * parameter);
    Path getPathOperator(rapidxml::xml_node<> * quantifyer_node);
    int max_depth(int a, int b);
    std::string loperator_sym(std::string loperator);
    CTLQuery * CopyQuery(CTLQuery *source);
    
    CTLQuery* FillAtom(CTLQuery* query, PetriEngine::PetriNet *net);
    CTLQuery* ConvertAG(CTLQuery* query);
    CTLQuery* ConvertEG(CTLQuery* query);
    CTLQuery* TemporalSetting(CTLQuery* query);
    int IdSetting(CTLQuery* query, int id);
    int GetNumberofChildren(rapidxml::xml_node<>* root);
    std::vector<CTLQuery*> *GetChildren(rapidxml::xml_node<>* node);
    std::string choppy( char *s );

};

#endif /* CTLPARSER_V2_H */

