/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EvaluateableProposition.h
 * Author: mossns
 *
 * Created on April 27, 2016, 2:36 PM
 */

#ifndef EVALUATEABLEPROPOSITION_H
#define EVALUATEABLEPROPOSITION_H
#include <vector>
#include "../../PetriEngine/PetriNet.h"


enum PropositionType {PropError = -1, CARDINALITY = 0, FIREABILITY = 1, DEADLOCK = 2};
enum ArithmeticType {NON = -1, SUM = 0, PRODUCT = 1, DIFF = 2};
enum LoperatorType {LopError = -1, NOT_CARDINALITY = -1, EQ = 0, LE = 1, LEQ = 2, GR = 3, GRQ = 4, NE = 5};

struct CardinalityParameter{
    bool isPlace = false;
    bool isArithmetic = false;
    int value = -1;
    std::vector<int> places_i;
    ArithmeticType arithmetictype = NON;
    CardinalityParameter* arithmA;
    CardinalityParameter* arithmB;
};

class EvaluateableProposition {
public:
    EvaluateableProposition(std::string a, PetriEngine::PetriNet *net);
    virtual ~EvaluateableProposition();
    PropositionType GetPropositionType();
    LoperatorType GetLoperator();
    std::vector<int> GetFireset();
    std::string ToString();
    CardinalityParameter* GetFirstParameter();
    CardinalityParameter* GetSecondParameter();
private:
    void SetFireset(std::string fireset_str, std::vector<std::string> t_names, unsigned int numberof_t);
    CardinalityParameter* CreateParameter(std::string parameter_str, std::vector<std::string> p_names, unsigned int numberof_p);
    LoperatorType SetLoperator(std::string atom_str);
    
    PropositionType _type = PropError;
    LoperatorType _loperator = LopError;
    std::vector<int> _fireset;
    CardinalityParameter* _firstParameter = nullptr;
    CardinalityParameter* _secondParameter = nullptr;
    
};

#endif /* EVALUATEABLEPROPOSITION_H */

