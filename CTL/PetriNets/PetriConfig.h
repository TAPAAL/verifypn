#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include <sstream>
#include "../DependencyGraph/Configuration.h"
#include "../CTLParser/CTLQuery.h"
#include "../CTLParser/CTLParser.h"

namespace PetriNets {

class PetriConfig : public DependencyGraph::Configuration {

public:

    PetriConfig(size_t t_marking, CTLQuery *t_query) :
        DependencyGraph::Configuration(), marking(t_marking), query(t_query) {
    }

    size_t marking;
    CTLQuery *query;

    virtual std::string toString() const override
    {
        std::stringstream ss;
        ss << "==================== Configuration ====================" << std::endl
           << attrToString() << " Depth: " << query->Depth << " IsTemporal: " << (query->IsTemporal ? "TRUE" : "FALSE") << std::endl
//           << marking->toString() << std::endl
           << query->ToString() << std::endl
           << "=======================================================" << std::endl;

        return ss.str();
    }

    virtual void printConfiguration() const override {
        std::cout << toString();
    }

};

}
#endif // PETRICONFIG_H
