#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/PQL/PQL.h"

#include <sstream>

namespace PetriNets {

class PetriConfig : public DependencyGraph::Configuration {

public:
    using Condition = PetriEngine::PQL::Condition;
    PetriConfig() : 
        DependencyGraph::Configuration(), marking(0), query(NULL) 
    {}
    
    PetriConfig(size_t t_marking, Condition *t_query) :
        DependencyGraph::Configuration(), marking(t_marking), query(t_query) {
    }

    virtual ~PetriConfig(){};

    size_t marking;
    Condition *query;

};

}
#endif // PETRICONFIG_H
