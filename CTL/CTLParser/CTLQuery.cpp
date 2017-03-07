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
    return TYPE_ERROR;
}

CTLQuery* CTLQuery::GetFirstChild(){
    if(_hasAtom){
        std::cout<<"Query " << ToString() << " does not have child"<<std::endl;
        throw 20;
    }
    return _firstchild;
}

CTLQuery* CTLQuery::GetSecondChild(){
    assert(!_hasAtom && "Query does not have children");
    assert(!(_path == F || _path == G || _path == X) && "Query does not have second child");
    assert(!(_q == NEG) && "Query does not have second child");
    return _secondchild;
}

void CTLQuery::SetFirstChild(CTLQuery *q){
    _firstchild = q;
}

void CTLQuery::SetSecondChild(CTLQuery *q){
    _secondchild = q;
}

Quantifier CTLQuery::GetQuantifier(){
    return _q;
}

Path CTLQuery::GetPath(){
    return _path;
}

std::string CTLQuery::GetAtom(){
    return _a;
}

EvaluateableProposition* CTLQuery::GetProposition(){
    assert(_hasAtom && "Query does not have proposition");
    return _proposition;
}

void CTLQuery::SetProposition(EvaluateableProposition *p){
    assert(_hasAtom && "Query does not have proposition");
    _proposition = p;
}

std::string CTLQuery::ToString(){
    if(_hasAtom){
        return _a;
    }
    else if(_q == AND){
        return " & ";
    }
    else if(_q == OR){
        return " | ";
    }
    else if(_q == NEG){
        return "!";
    }
    else if(_q == A || _q == E){
        std::string quanti = "";
        if (_q == A){
            quanti = "A";
        }
        else quanti = "E";
        if(_path == F){
            return quanti + "F";
        }
        else if(_path == G){
            return quanti + "G";
        }
        else if(_path == X){
            return quanti + "X";
        }
        else if(_path == U){
            
            return quanti.append("U");
        }
        else{
            std::cout<<"::::::Error Printing: Path was empty::::"<<std::endl;
            assert(false);
        }
    }
    else{
        std::cout<<"::::::Error Printing: Q was empty::::"<<std::endl;
        assert(false);
    }
    exit(EXIT_FAILURE);
}



