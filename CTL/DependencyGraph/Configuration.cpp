#include "Configuration.h"
//#include <stdlib.h> //abs
#include <iostream>
#include <sstream>

DependencyGraph::Configuration::~Configuration() {
    for(Edge *e : successors)
        delete e;
    for(Edge *e : deleted_successors)
        delete e;
}

void DependencyGraph::Configuration::removeSuccessor(DependencyGraph::Edge *t_successor){
    auto iter = successors.begin();
    auto end = successors.end();

    while(iter != end){
        if(*iter == t_successor){
            deleted_successors.insert(deleted_successors.end(), *iter);
            successors.erase(iter);
            successors.shrink_to_fit();
            deleted_successors.shrink_to_fit();
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

void DependencyGraph::Configuration::updateInterest(int worker, long id)
{
    bool found = false;
    for (int i=0; i<interested.size(); i++) {
        if (interested[i].first == worker) {
            found = true;
            if (abs(interested[i].second) < abs(id)) {
                interested[i] = std::pair<int, long>(worker, id);
            }
        }
    }
    if (!found) {
        interested.push_back(std::pair<int, long>(worker, id));
    }
}

bool DependencyGraph::Configuration::hasActiveDependencies()
{
    if (!dependency_set.empty()) return true;
    for (std::pair<int, long> interest : interested) {
        if (interest.second > 0) return true;
    }
    return false;
}

