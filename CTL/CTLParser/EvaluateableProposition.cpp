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

        size_t begin = a.find('(') + 1;
        size_t end = a.find(')') - begin;
        std::string first_parameter_str = a.substr(begin, end);
        a = a.substr(a.find(')') + 1);

        begin = a.find('(') + 1;
        end = a.find(')') - begin;
        std::string second_parameter_str = a.substr(begin, end);
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
        assert(false && "Atomic string proposed for proposition could not be parsed");
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

LoperatorType EvaluateableProposition::SetLoperator(std::string atom_str){
    std::string loperator_str = atom_str.substr(atom_str.find(')') + 1);
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
        std::string cardi_str = "(";
        if(_firstParameter->isPlace)
            cardi_str = cardi_str + "place(" + patch::to_string(_firstParameter->value) + ")";
        else
            cardi_str = cardi_str = patch::to_string(_firstParameter->value);
        
        cardi_str = cardi_str + " loperator ";
        
        if(_secondParameter->isPlace)
            cardi_str = cardi_str + "place(" + patch::to_string(_secondParameter->value) + ")";
        else
            cardi_str = cardi_str = patch::to_string(_secondParameter->value);
        return cardi_str + ")";
    }
    else
        std::cerr << "Error: An unknown error occured while converting a proposition to string. " << std::endl;
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
