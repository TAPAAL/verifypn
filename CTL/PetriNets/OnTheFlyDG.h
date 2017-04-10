#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include <functional>

#include "../DependencyGraph/BasicDependencyGraph.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"
#include "../CTLParser/CTLQuery.h"
#include "PetriConfig.h"
#include "../../PetriParse/PNMLParser.h"
#include "PetriEngine/Structures/ptrie_map.h"
#include "PetriEngine/Structures/AlignedEncoder.h"
#include "PetriEngine/Structures/linked_bucket.h"

namespace PetriNets {

class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph
{
public:
    typedef PetriEngine::Structures::State Marking;
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

    //stats
    int configurationCount() const;
    int markingCount() const;

protected:

    //initialized from constructor
    AlignedEncoder encoder;
    PetriEngine::PetriNet *net = nullptr;
    PetriConfig* initial_config;
    Marking working_marking;
    Marking query_marking;
    uint32_t n_transitions = 0;
    uint32_t n_places = 0;
    size_t _markingCount = 0;
    size_t _configurationCount = 0;

    //used after query is set
    CTLQuery *query = nullptr;

    bool evaluateQuery(CTLQuery &query, size_t marking, Marking* unfolded);
    bool fastEval(CTLQuery &query, size_t marking, Marking* unfolded);
    void nextStates(Marking& t_marking, 
    std::function<void ()> pre, 
    std::function<bool (size_t, Marking&)> foreach, 
    std::function<void ()> post);
    bool EvalCardianlity(int a, LoperatorType lop, int b);
    int GetParamValue(CardinalityParameter *param, Marking& marking);
    PetriConfig *createConfiguration(size_t marking, CTLQuery &query);
    size_t createMarking(Marking &marking);
    void markingStats(const uint32_t* marking, size_t& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last);
    
    DependencyGraph::Edge* newEdge(DependencyGraph::Configuration &t_source);

    ptrie::map<std::vector<PetriConfig*> > trie;
    linked_bucket_t<PetriConfig,1024*10> config_alloc;
    linked_bucket_t<DependencyGraph::Edge,1024*10> edge_alloc;
    
};
}
#endif // ONTHEFLYDG_H
