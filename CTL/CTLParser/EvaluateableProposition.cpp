/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EvaluateableProposition.cpp
 * Author: mossns
 * 
 * Created on April 27, 2016, 2:36 PM
 */

#include <string>
#include <stdexcept> 
#include <sstream>
#include <cstdint>
#include <algorithm>

#include "EvaluateableProposition.h"
#include "CTLParser.h"

namespace patch
{
    //A replacement for the standard to string function, which we cannot use because old OS X does not have c++11
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

EvaluateableProposition::EvaluateableProposition(std::string a, PetriEngine::PetriNet *net) {
    if(a.substr(0,2).compare("in") == 0 || a.substr(0,2).compare("to") == 0){
        _type = CARDINALITY;
        _loperator = SetLoperator(a);
        assert(_loperator != NOT_CARDINALITY);

        //size_t begin = a.find('(') + 1;
        size_t end = a.find('}');
        std::string first_parameter_str = a.substr(0, end);
        a = a.substr(a.find('}') + 1);

        size_t begin = a.find('{') + 1;
        //end = a.find(')') - begin;
        std::string second_parameter_str = a.substr(begin);
        _firstParameter = CreateParameter(first_parameter_str, net->placeNames(), net->numberOfPlaces());
        _secondParameter = CreateParameter(second_parameter_str, net->placeNames(), net->numberOfPlaces());
    }
    else if(a.substr(0,2).compare("is") == 0){
        _type = FIREABILITY;
        _loperator = NOT_CARDINALITY;
        size_t s_pos = a.find('(') + 1;
        size_t e_pos = a.find(')') - 1;
        assert(s_pos < e_pos);
        int fire_str_length = e_pos - s_pos + 1;
        std::string fireset_str = a.substr(s_pos, fire_str_length);
        SetFireset(fireset_str, net->transitionNames(), net->numberOfTransitions());
    }
    else if(a.substr(0,2).compare("tr") == 0){
        _type = CARDINALITY;
        _loperator = EQ;
        
        _firstParameter = new CardinalityParameter();
        _secondParameter = new CardinalityParameter();
        
        _firstParameter->isPlace = false;
        _firstParameter->value = 0;
        
        _secondParameter->isPlace = false;
        _secondParameter->value = 0;
    }
    else if(a.substr(0,2).compare("fa") == 0){
        _type = CARDINALITY;
        _loperator = EQ;
        
        _firstParameter = new CardinalityParameter();
        _secondParameter = new CardinalityParameter();
        
        _firstParameter->isPlace = false;
        _firstParameter->value = 0;
        
        _secondParameter->isPlace = false;
        _secondParameter->value = 1;
    }
    else if(a.substr(0,2).compare("de") == 0){
        _type = DEADLOCK;
        _loperator = NOT_CARDINALITY;
        SetFireset("all", net->transitionNames(), net->numberOfTransitions());
    }
    else{
        std::cerr << "Error: Could not identify Evaluateable Proposition type from atom string: "<<a<<"." << std::endl;
        exit(EXIT_FAILURE);
    }
}

EvaluateableProposition::~EvaluateableProposition() {
}

PropositionType EvaluateableProposition::GetPropositionType(){
    return _type;
}

LoperatorType EvaluateableProposition::GetLoperator(){
    assert(_type == CARDINALITY && "Proposition is not a cardinality proposition");
    return _loperator;
}

std::vector<int> EvaluateableProposition::GetFireset() {
    assert((_type == FIREABILITY || _type == DEADLOCK) && "Proposition is not a fireability proposition");
    return _fireset;
}

void EvaluateableProposition::SetFireset(std::string fireset_str, std::vector<std::string> t_names, unsigned int numberof_t){
    std::string restof_firestring = fireset_str;
    
    if(fireset_str.compare("all") == 0){
        for(uint i = 0; i < numberof_t; i++){
            _fireset.push_back(i);
        }
        return;
    }
    
    while(restof_firestring.length() > 0){
        size_t position = restof_firestring.find(',');
        std::string current_t = restof_firestring.substr(0, position);
        current_t.erase(std::remove_if(current_t.begin(), current_t.end(), isspace), current_t.end());
        for(uint i = 0; i < numberof_t; i++){
            if (current_t.compare(t_names[i]) == 0){
                _fireset.push_back(i);
            }
        }
        if (position != SIZE_MAX){
            restof_firestring = restof_firestring.substr(position + 1);
        }
        else
            restof_firestring = "";
    }
}

CardinalityParameter* EvaluateableProposition::CreateParameter(std::string parameter_str, std::vector<std::string> p_names, unsigned int numberof_p){
    CardinalityParameter *param = new CardinalityParameter();
    while(parameter_str.substr(0,1).compare(" ")==0){
        parameter_str = parameter_str.substr(1);
    }
    //std::cout<<"ParamString: "<<parameter_str<<"\n"<<std::flush;

    ParameterHolder* typeHolder = new ParameterHolder();
    if(parameter_str.substr(0,11).compare("integer-sum")==0){
        typeHolder->type = SUM;
        typeHolder->body = parameter_str.substr(11);
    }
    else if(parameter_str.substr(0,15).compare("integer-product")==0){
        typeHolder->type = PRODUCT;
        typeHolder->body = parameter_str.substr(15);
    }
    else if(parameter_str.substr(0,18).compare("integer-difference")==0){
        typeHolder->type = DIFF;
        typeHolder->body = parameter_str.substr(18);
    }
    else if(parameter_str.substr(0,16).compare("integer-constant")==0){
        typeHolder->type = CONST;
        typeHolder->body = parameter_str.substr(16);
    }
    else if(parameter_str.substr(0,12).compare("tokens-count")==0){
        typeHolder->type = PLACE;
        typeHolder->body = parameter_str.substr(12);
    }
    else{
        std::cerr << "Error: Internal parse error - \"" << parameter_str <<"\" has unrecognized beginning." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::size_t found = typeHolder->body.find_last_of(")");
    parameter_str = typeHolder->body.substr(1,found-1);

    if(typeHolder->type != CONST && typeHolder->type != PLACE){
        std::size_t end_of_first = GetEndingParamIndex(parameter_str) + 1;
        param->isArithmetic = true;
        param->arithmetictype = typeHolder->type;
        param->arithmA = CreateParameter(parameter_str.substr(0,end_of_first), p_names, numberof_p);
        param->arithmB = CreateParameter(parameter_str.substr(end_of_first + 2), p_names, numberof_p);
        return param;
    }


    char c;
    if(sscanf(parameter_str.c_str(), "%d%c", &param->value, &c) == 1) {
        //If string is identifier starting with a number, you will read two items.
        //If it's an identifier starting with a character, you will read zero items.
        //The only time when you read just one item if the whole string is just numbers.
        param->isPlace = false;
    } else {    //error
        
        param->isPlace = true;
        std::vector<std::string> places_str;
        std::size_t found = parameter_str.find(",");
        
        while(found != parameter_str.npos){
            std::string temp = parameter_str.substr(0, found);
            places_str.push_back(temp);
            parameter_str = parameter_str.substr(found + 2);
            found = parameter_str.find(",");
        }
        
        places_str.push_back(parameter_str);
        
        uint found_count = 0;
        for(uint i = 0; i < numberof_p; i++){
            for(std::string place : places_str){
                if(p_names[i].compare(place) == 0){
                    param->places_i.push_back(i);
                    found_count++;
                }
            }
        }
        if (found_count != places_str.size()) {
            std::cerr << "Error: The query file contains identifiers that are not in the net. Please check spelling." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return param;
}

std::size_t EvaluateableProposition::GetEndingParamIndex(std::string param_str){
    size_t pos = param_str.find('(');
    size_t start_count =1;
    size_t end_count =0;
    std::string temp = param_str.substr(pos+1);
    while(start_count != end_count){
        size_t new_pos = 0;
        if(temp.find('(') < temp.find(')')){
            start_count++;
            new_pos = temp.find('(') + 1;
        }
        else{
            end_count++;
            new_pos = temp.find(')') + 1;
        }
        pos = pos + new_pos;
        temp = temp.substr(new_pos);
    }

    return pos;
}

LoperatorType EvaluateableProposition::SetLoperator(std::string atom_str){
    std::string loperator_str = atom_str.substr(atom_str.find('}') + 1);
    loperator_str = loperator_str.substr(0, 4);
    if(loperator_str.compare(" le ")== 0)
            return LEQ;
    else if (loperator_str.compare(" ge ")== 0)
        return GRQ;
    else if (loperator_str.compare(" eq ")== 0)
        return EQ;
    else if (loperator_str.compare(" gr ")== 0)
        return GR;
    else if (loperator_str.compare(" ls ")== 0)
        return LE;
    else if (loperator_str.compare(" ne ")== 0)
        return NE;
    else std::cerr << "Error: Could not parse the logical operator: \"" << loperator_str << "\"" << std::endl;
    exit(EXIT_FAILURE);
}

std::string EvaluateableProposition::ToString() {
    if (_type == FIREABILITY) {
        std::string fire_str = "Fireset(";
        for(auto f : _fireset){
            fire_str = fire_str + " " + patch::to_string(f);
        }
        return fire_str + ")";
    }
    else if (_type == CARDINALITY){
        return Parameter_tostring(_firstParameter) + Loperator_tostring() + Parameter_tostring(_secondParameter);
    }
    std::cerr << "Error: An unknown error occured while converting a proposition to string. " << std::endl;
    exit(EXIT_FAILURE);
}


std::string EvaluateableProposition::Parameter_tostring(CardinalityParameter* param){
    std::string cardi_str = "";
    if(param->isPlace){
        cardi_str = cardi_str + "place(";
        int i = 0;
        while(i != param->places_i.size()){
            cardi_str = cardi_str + patch::to_string(param->places_i[i]);

            if(++i < param->places_i.size()){
                cardi_str = cardi_str + ", ";
            }
        }
        cardi_str = cardi_str + ")";
    }
    else if(param->isArithmetic){
        std::string arthm_ope = "";
        if(param->arithmetictype == SUM)
            arthm_ope = " + ";
        else if (param->arithmetictype == PRODUCT)
            arthm_ope = " * ";
        else if (param->arithmetictype == DIFF)
            arthm_ope = " - ";
        else {
            std::cerr << "Error: An unknown error occured while converting a proposition to string. " << std::endl;
            exit(EXIT_FAILURE);
        }
        cardi_str = cardi_str + Parameter_tostring(param->arithmA) + arthm_ope + Parameter_tostring(param->arithmB);
    }
    else
        cardi_str = cardi_str = patch::to_string(param->value);
    return cardi_str;
}

std::string EvaluateableProposition::Loperator_tostring(){
    if(_loperator == LEQ)
            return " <= ";
    else if (_loperator == GRQ)
        return " >= ";
    else if (_loperator == EQ)
        return " == ";
    else if (_loperator == GR)
        return " > ";
    else if (_loperator == LE)
        return " < ";
    else if (_loperator == NE)
        return " != ";
    else std::cerr << "Error: Could not convert loperator to string" << std::endl;
    exit(EXIT_FAILURE);
}

CardinalityParameter* EvaluateableProposition::GetFirstParameter() {
    assert(_type == CARDINALITY);
    return _firstParameter;
}

CardinalityParameter* EvaluateableProposition::GetSecondParameter() {
    assert(_type == CARDINALITY);
    return _secondParameter;
}
