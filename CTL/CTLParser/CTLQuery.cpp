/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLQuery.cpp
 * Author: mossns
 * 
 * Created on April 22, 2016, 7:57 AM
 */

#include <string.h>

#include "CTLQuery.h"
#include "CTLParser.h"

CTLQuery::CTLQuery(Quantifier q, Path p, bool isAtom, std::string atom_str) {
    if(isAtom){
        assert(q == EMPTY);
        assert(p == pError);
        assert(atom_str.compare("") != 0);
        
        _hasAtom = true;
        _hasQuantifier = false;
        _hasPath = false;
        _q = q;
        _path = p;
        _a = atom_str;
        IsTemporal = false;
    }
    else if(q == AND || q == OR || q == NEG){
        assert(p == pError);
        _hasAtom = false;
        _hasQuantifier = true;
        _hasPath = false;
        _q = q;
        _path = p;
        _a = "";
        IsTemporal = false;
    }
    else{
        assert(q != EMPTY);
        assert(p != pError);
        _hasAtom = false;
        _hasQuantifier = true;
        _hasPath = true;
        _q = q;
        _path = p;
        _a = "";
        IsTemporal = true;
    }
    
}

CTLQuery::~CTLQuery() {
}

CTLType CTLQuery::GetQueryType(){
    if(_hasAtom)
        return EVAL;
    else if(_path != pError)
        return PATHQEURY;
    else if(_q == AND || _q == OR || _q == NEG)
        return LOPERATOR;
    std::cerr << "Error: Query Type Error - Query does not have a defined type, but the type of the query was requested." << std::endl;
    exit(EXIT_FAILURE);
}

CTLQuery* CTLQuery::GetFirstChild(){
    if(_hasAtom){
        std::cerr<<"Query does not have child"<<std::endl;
        exit(EXIT_FAILURE);
    }
    return _firstchild;
}

CTLQuery* CTLQuery::GetSecondChild(){
    if(_hasAtom || _path == F || _path == G || _path == X){
        std::cerr<<"Query does not have child"<<std::endl;
        exit(EXIT_FAILURE);
    }
    return _secondchild;
}

void CTLQuery::SetFirstChild(CTLQuery *q){
    if(_hasAtom){
        std::cerr<<"Query cannot be given a child"<<std::endl;
        exit(EXIT_FAILURE);
    }
    _firstchild = q;
}

void CTLQuery::SetSecondChild(CTLQuery *q){
    if(_hasAtom){
        std::cerr<<"Query cannot be given a child"<<std::endl;
        exit(EXIT_FAILURE);
    }
    _secondchild = q;
}

Quantifier CTLQuery::GetQuantifier(){
    return _q;
}

Path CTLQuery::GetPath(){
    return _path;
}

std::string CTLQuery::GetAtom(){
    if (!_hasAtom){
        std::cerr<<"Query does not have an atom " <<std::endl;
        exit(EXIT_FAILURE);
    }
    return _a;
}

EvaluateableProposition* CTLQuery::GetProposition(){
    if (!_hasAtom){
        std::cerr<<"Query does not have proposition " <<std::endl;
        exit(EXIT_FAILURE);
    }
    return _proposition;
}

void CTLQuery::SetProposition(EvaluateableProposition *p){
    if (!_hasAtom){
        std::cerr<<"Query cannot be given a proposition " <<std::endl;
        exit(EXIT_FAILURE);
    }
    _proposition = p;
}

std::string CTLQuery::ToString(){
    if(_hasAtom){
        return _a;
    }
    else if(_q == AND){
        return "(" + _firstchild->ToString() + " & " +  _secondchild->ToString() + ")";
    }
    else if(_q == OR){
        return "(" + _firstchild->ToString() + " | " +  _secondchild->ToString() + ")";
    }
    else if(_q == NEG){
        return "!(" + _firstchild->ToString() + ")";
    }
    else if(_q == A || _q == E){
        std::string quanti = "";
        if (_q == A){
            quanti = "A";
        }
        else quanti = "E";
        if(_path == F){
            return quanti + "F" + _firstchild->ToString();
        }
        else if(_path == G){
            return quanti + "G" + _firstchild->ToString();
        }
        else if(_path == X){
            return quanti + "X" + _firstchild->ToString();
        }
        else if(_path == U){
            return quanti + "("  + _firstchild->ToString() + ") U (" + _secondchild->ToString() + ")";
        }
        else{
            std::cerr<<"::::::Error Printing: Path was empty::::"<<std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else{
        std::cerr<<"::::::Error Printing: Q was empty::::"<<std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
}



