#include "Configuration.h"
//#include <stdlib.h> //abs
#include <iostream>
#include <sstream>
#include <complex>

DependencyGraph::Configuration::~Configuration() {
    for(Edge *e : successors)
        delete e;
}

void DependencyGraph::Configuration::removeSuccessor(DependencyGraph::Edge *t_successor){
    auto iter = successors.begin();
    auto end = successors.end();

    while(iter != end){
        if(*iter == t_successor){
            successors.erase(iter);
            successors.shrink_to_fit();
            break;
        }
        else
            iter++;
    }
}

std::string DependencyGraph::Configuration::toString() const
{
    std::stringstream ss;
    ss << "==================== Configuration ====================" << std::endl
       << attrToString()
       << "=======================================================" << std::endl;

    return ss.str();
}

std::string DependencyGraph::Configuration::attrToString() const{
    std::stringstream ss;

    ss << "Addr: " << this << " Assignment: " << assignmentToStr(assignment) << " Negated: " << "<void>";

    return ss.str();
}

void DependencyGraph::Configuration::printConfiguration() const{
    std::cout << toString();
}

std::string DependencyGraph::Configuration::assignmentToStr(DependencyGraph::Assignment a){
    if(a == ONE)
        return std::string("ONE");
    else if(a == UNKNOWN)
        return std::string("UNKNOWN");
    else if(a == ZERO)
        return std::string("ZERO");
    else
        return std::string("CZERO");
}


