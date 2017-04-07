/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLQuery.h
 * Author: Søren Moss Nielsen
 *
 * Created on April 22, 2016, 7:57 AM
 */

#ifndef CTLQUERY_H
#define CTLQUERY_H
#include "EvaluateableProposition.h"

enum CTLType {PATHQEURY = 1, LOPERATOR = 2, EVAL = 3, TYPE_ERROR = -1};
enum Quantifier { AND = 1, OR = 2, A = 3, E = 4, NEG = 5, EMPTY = -1 };
enum Path { G = 1, X = 2, F = 3, U = 4, pError = -1 };

class CTLQuery {
    
public:
    CTLQuery(Quantifier q, Path p, bool isAtom, std::string atom_str);
    virtual ~CTLQuery();
    
    int Id = -1;
    int Depth = -1;
    
    CTLType GetQueryType();
    CTLQuery* GetFirstChild();
    CTLQuery* GetSecondChild();
    void SetFirstChild(CTLQuery *q);
    void SetSecondChild(CTLQuery *q);
    std::string ToString();
    
    Quantifier GetQuantifier();
    Path GetPath();
    std::string GetAtom();
    
    EvaluateableProposition* GetProposition();
    void SetProposition(EvaluateableProposition *p);
    
    bool IsTemporal = false;
    
    
private:
    std::string CreateEvaluateableProposition(std::string a);
    
    bool _hasQuantifier = false;
    bool _hasPath = false;
    bool _hasAtom =false;
    Quantifier _q;
    Path _path;
    std::string _a;
    
    CTLQuery* _firstchild = nullptr;
    CTLQuery* _secondchild = nullptr;
    
    EvaluateableProposition* _proposition = nullptr;
};

#endif /* CTLQUERY_H */

