#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "../DependencyGraph/BasicDependencyGraph.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"
#include "../CTLParser/CTLQuery.h"
#include "PetriConfig.h"
#include "Marking.h"
#include "../../PetriParse/PNMLParser.h"

#include <unordered_set>
#include <list>

namespace PetriNets {

class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph
{
private:
    struct MarkingEqualityHasher{
        MarkingEqualityHasher(){}
        MarkingEqualityHasher(PetriEngine::PetriNet *net){length = net->numberOfPlaces();}
        uint32_t length = 0;
        bool operator()(const Marking* rhs, const Marking* lhs) const{
            for(uint32_t i = 0; i < length; i++){
                if((*rhs)[i] != (*lhs)[i]){
                    return false;
                }
            }
            return true;
        }
        size_t operator()(const PetriNets::Marking* pmarking ) const {
            size_t hash = 0;
            uint32_t& h1 = ((uint32_t*)&hash)[0];
            uint32_t& h2 = ((uint32_t*)&hash)[1];
            uint32_t cnt = 0;
            for (size_t i = 0; i < length; i++)
            {
                if((*pmarking)[i])
                {
                    h1 ^= 1 << (i % 32);
                    h2 ^= (*pmarking)[i] << (cnt % 32);
                    ++cnt;
                }
            }
            return hash;
        }
    };
    typedef std::unordered_set<Marking*, MarkingEqualityHasher, MarkingEqualityHasher> MarkingContainer;
public:
    OnTheFlyDG(PetriEngine::PetriNet *t_net);

    virtual ~OnTheFlyDG();

    //Dependency graph interface
    virtual void successors(DependencyGraph::Configuration *c) override;
    virtual DependencyGraph::Configuration *initialConfiguration() override;
    virtual void cleanUp() override;

    //Serializer interface
//    virtual std::pair<int, int*> serialize(SearchStrategy::Message &m) override;
//    virtual SearchStrategy::Message deserialize(int* message, int messageSize) override;

    void setQuery(CTLQuery* query);
    Marking * initialMarking() {return initial_marking;}

    //stats
    int configurationCount() const;
    int markingCount() const;

protected:

    //initialized from constructor
    PetriEngine::PetriNet *net = nullptr;
    Marking *initial_marking = new Marking();
    uint32_t n_transitions = 0;
    uint32_t n_places = 0;

    //used after query is set
    DependencyGraph::Configuration *initial = nullptr;
    std::vector<Marking*> cached_successors;
    Marking *cached_marking = nullptr;
    CTLQuery *query = nullptr;

    bool evaluateQuery(CTLQuery &query, Marking &marking);
    bool fastEval(CTLQuery &query, Marking &marking);
    std::vector<Marking*> nextState(Marking &marking);
    bool EvalCardianlity(int a, LoperatorType lop, int b);
    int GetParamValue(CardinalityParameter *param, Marking& marking);
    DependencyGraph::Configuration *createConfiguration(Marking &marking, CTLQuery &query);
    Marking *createMarking(const Marking &marking);

    CTLQuery *findQueryById(int id, CTLQuery *root);

    MarkingContainer markings;
};
}
#endif // ONTHEFLYDG_H
