#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "../DependencyGraph/BasicDependencyGraph.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"
#include "../CTLParser/CTLQuery.h"
#include "PetriConfig.h"
#include "Marking.h"
#include "../../PetriParse/PNMLParser.h"
#include "PetriEngine/Structures/ptrie_map.h"
#include "PetriEngine/Structures/AlignedEncoder.h"

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

    //stats
    int configurationCount() const;
    int markingCount() const;

protected:

    //initialized from constructor
    AlignedEncoder encoder;
    PetriEngine::PetriNet *net = nullptr;
    size_t initial_marking;
    Marking working_marking;
    Marking query_marking;
    uint32_t n_transitions = 0;
    uint32_t n_places = 0;
    size_t _markingCount = 0;
    size_t _configurationCount = 0;

    //used after query is set
    CTLQuery *query = nullptr;

    bool evaluateQuery(CTLQuery &query, size_t marking);
    bool fastEval(CTLQuery &query, size_t marking);
    std::vector<size_t> nextStates(size_t marking);
    bool EvalCardianlity(int a, LoperatorType lop, int b);
    int GetParamValue(CardinalityParameter *param, Marking& marking);
    DependencyGraph::Configuration *createConfiguration(size_t marking, CTLQuery &query);
    size_t createMarking(Marking &marking);
    void markingStats(const uint32_t* marking, size_t& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last);

    MarkingContainer markings;
    ptrie::map<std::vector<PetriConfig*> > trie;
};
}
#endif // ONTHEFLYDG_H
