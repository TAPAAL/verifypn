#include "CTL/PetriNets/OnTheFlyDG.h"

#include <algorithm>
#include <string.h>
#include <iostream>
#include <queue>
#include <limits>

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PQL/Expressions.h"
#include "CTL/SearchStrategy/SearchStrategy.h"


using namespace PetriEngine::PQL;
using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net, bool partial_order) : encoder(t_net->numberOfPlaces(), 0), 
        edge_alloc(new linked_bucket_t<DependencyGraph::Edge,1024*10>(1)), 
        conf_alloc(new linked_bucket_t<char[sizeof(PetriConfig)], 1024*1024>(1)),
        _redgen(*t_net), _partial_order(partial_order) {
    net = t_net;
    n_places = t_net->numberOfPlaces();
    n_transitions = t_net->numberOfTransitions();
}


OnTheFlyDG::~OnTheFlyDG()
{
    cleanUp();
    //Note: initial marking is in the markings set, therefore it will be deleted by the for loop
    //TODO: Ensure we don't leak memory here, when code moving is done
    size_t s = conf_alloc->size();
    for(size_t i = 0; i < s; ++i)
    {
        ((PetriConfig*)&(*conf_alloc)[i])->~PetriConfig();
    }
    delete conf_alloc;
    delete edge_alloc;
}

Condition::Result OnTheFlyDG::initialEval()
{
    initialConfiguration();
    EvaluationContext e(query_marking.marking(), net);
    return query->evaluate(e);
}

Condition::Result OnTheFlyDG::fastEval(Condition* query, Marking* unfolded)
{
    EvaluationContext e(unfolded->marking(), net);
    return query->evaluate(e);
}


std::vector<DependencyGraph::Edge*> OnTheFlyDG::successors(Configuration *c)
{
    PetriEngine::PQL::DistanceContext context(net, query_marking.marking());
    PetriConfig *v = static_cast<PetriConfig*>(c);
    trie.unpack(v->marking, encoder.scratchpad().raw());
    encoder.decode(query_marking.marking(), encoder.scratchpad().raw());
    //    v->printConfiguration();
    std::vector<Edge*> succs;
    auto query_type = v->query->getQueryType();
    if(query_type == EVAL){
        assert(false);
        //assert(false && "Someone told me, this was a bad place to be.");
        if (fastEval(query, &query_marking) == Condition::RTRUE){
            succs.push_back(newEdge(*v, 0));///*v->query->distance(context))*/0);
        }
    }
    else if (query_type == LOPERATOR){
        if(v->query->getQuantifier() == NEG){
            // no need to try to evaluate here -- this is already transient in other evaluations.
            auto cond = static_cast<NotCondition*>(v->query);
            Configuration* c = createConfiguration(v->marking, v->owner, (*cond)[0]);
            Edge* e = newEdge(*v, /*v->query->distance(context)*/0);
            e->is_negated = true;
            e->addTarget(c);
            succs.push_back(e);
        }
        else if(v->query->getQuantifier() == AND){
            auto cond = static_cast<AndCondition*>(v->query);
            //Check if left is false
            std::vector<Condition*> conds;
            for(auto& c : *cond)
            {
                auto res = fastEval(c.get(), &query_marking);
                if(res == Condition::RFALSE)
                {
                    return succs;
                }
                if(res == Condition::RUNKNOWN)
                {
                    conds.push_back(c.get());
                }
            }
            
            Edge *e = newEdge(*v, /*cond->distance(context)*/0);

            //If we get here, then either both propositions are true (shouldn't be possible)
            //Or a temporal operator and a true proposition
            //Or both are temporal
            for(auto c : conds)
            {
                assert(c->isTemporal());
                e->addTarget(createConfiguration(v->marking, v->owner, c));
            }
            succs.push_back(e);
        }
        else if(v->query->getQuantifier() == OR){
            auto cond = static_cast<OrCondition*>(v->query);
            //Check if left is true
            std::vector<Condition*> conds;
            for(auto& c : *cond)
            {
                auto res = fastEval(c.get(), &query_marking);
                if(res == Condition::RTRUE)
                {
                    succs.push_back(newEdge(*v, 0));
                    return succs;
                }
                if(res == Condition::RUNKNOWN)
                {
                    conds.push_back(c.get());
                }
            }

            //If we get here, either both propositions are false
            //Or one is false and one is temporal
            //Or both temporal
            for(auto c : conds)
            {
                assert(c->isTemporal());
                Edge *e = newEdge(*v, /*cond->distance(context)*/0);
                e->addTarget(createConfiguration(v->marking, v->owner, c));
                succs.push_back(e);
            }
        }
        else{
            assert(false && "An unknown error occoured in the loperator-part of the successor generator");
        }
    }
    else if (query_type == PATHQEURY){
        if(v->query->getQuantifier() == A){
            if (v->query->getPath() == U){
                auto cond = static_cast<AUCondition*>(v->query);
                Edge *right = NULL;       
                auto r1 = fastEval((*cond)[1], &query_marking);
                if (r1 != Condition::RUNKNOWN){
                    //right side is not temporal, eval it right now!
                    if (r1 == Condition::RTRUE) {    //satisfied, no need to go through successors
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }//else: It's not valid, no need to add any edge, just add successors
                }
                else {
                    //right side is temporal, we need to evaluate it as normal
                    Configuration* c = createConfiguration(v->marking, v->owner, (*cond)[1]);
                    right = newEdge(*v, /*(*cond)[1]->distance(context)*/0);
                    right->addTarget(c);
                }
                bool valid = false;
                Configuration *left = NULL;
                auto r0 = fastEval((*cond)[0], &query_marking);
                if (r0 != Condition::RUNKNOWN) {
                    //left side is not temporal, eval it right now!
                    valid = r0 == Condition::RTRUE;
                } else {
                    //left side is temporal, include it in the edge
                    left = createConfiguration(v->marking, v->owner, (*cond)[0]);
                }
                if (valid || left != NULL) {
                    //if left side is guaranteed to be not satisfied, skip successor generation
                    Edge* leftEdge = NULL;
                    nextStates (query_marking, cond,
                                [&](){ leftEdge = newEdge(*v, std::numeric_limits<uint32_t>::max());},
                                [&](Marking& mark){
                                    auto res = fastEval(cond, &mark);
                                    if(res == Condition::RTRUE) return true;
                                    if(res == Condition::RFALSE)
                                    {
                                        left = nullptr;
                                        leftEdge->targets.clear();
                                        leftEdge = nullptr;
                                        return false;
                                    }
                                    context.setMarking(mark.marking());
                                    leftEdge->weight = 0;//std::min(leftEdge->weight, /*cond->distance(context)*/0);
                                    Configuration* c = createConfiguration(createMarking(mark), owner(mark, cond), cond);
                                    leftEdge->addTarget(c);
                                    return true;
                                },
                                [&]()
                                {
                                    if(leftEdge)
                                    {
                                        if (left != NULL) {
                                            leftEdge->addTarget(left);
                                        }
                                        succs.push_back(leftEdge);                                    
                                    }
                                }
                            );
                } //else: Left side is not temporal and it's false, no way to succeed there...

                if (right != NULL) {
                    succs.push_back(right);
                }
            }
            else if(v->query->getPath() == F){
                auto cond = static_cast<AFCondition*>(v->query);
                Edge *subquery = NULL;
                auto r = fastEval((*cond)[0], &query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }
                } else {
                    subquery = newEdge(*v, /*cond->distance(context)*/0);
                    Configuration* c = createConfiguration(v->marking, v->owner, (*cond)[0]);
                    subquery->addTarget(c);
                }
                Edge* e1 = NULL;
                nextStates(query_marking, cond,
                        [&](){e1 = newEdge(*v, std::numeric_limits<uint32_t>::max());},
                        [&](Marking& mark)
                        {
                            auto res = fastEval(cond, &mark);
                            if(res == Condition::RTRUE) return true;
                            if(res == Condition::RFALSE)
                            {
                                if(subquery)
                                {
                                    --subquery->refcnt;
                                    release(subquery);
                                    subquery = nullptr;
                                }
                                e1->targets.clear();
                                return false;
                            }
                            context.setMarking(mark.marking());
                            e1->weight = 0;//std::min(e1->weight, /*cond->distance(context)*/0);
                            Configuration* c = createConfiguration(createMarking(mark), owner(mark, cond), cond);
                            e1->addTarget(c);
                            return true;
                        },
                        [&]()
                        {
                            succs.push_back(e1);
                        }                    
                );

                if (subquery != nullptr) {
                    succs.push_back(subquery);
                }
            }
            else if(v->query->getPath() == X){
                auto cond = static_cast<AXCondition*>(v->query);
                Edge* e = newEdge(*v, std::numeric_limits<uint32_t>::max());
                Condition::Result allValid = Condition::RTRUE;
                nextStates(query_marking, cond,
                        [](){}, 
                        [&](Marking& mark){
                            auto res = fastEval((*cond)[0], &mark);
                            if(res != Condition::RUNKNOWN)
                            {
                                if (res == Condition::RFALSE) {
                                    allValid = Condition::RFALSE;
                                    return false;
                                }
                            }
                            else
                            {
                                allValid = Condition::RUNKNOWN;
                                context.setMarking(mark.marking());
                                e->weight = 0;//std::min(e->weight, /*cond->distance(context)*/0);
                                Configuration* c = createConfiguration(createMarking(mark), v->owner, (*cond)[0]);
                                e->addTarget(c);
                            }
                            return true;
                        }, 
                        [](){}
                    );
                    if(allValid == Condition::RUNKNOWN)
                    {
                        succs.push_back(e);
                    }
                    else if(allValid == Condition::RTRUE)
                    {
                        e->targets.clear();
                        succs.push_back(e);
                    }
                    else
                    {
                    }
            }
            else if(v->query->getPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
        else if(v->query->getQuantifier() == E){
            if (v->query->getPath() == U){
                auto cond = static_cast<EUCondition*>(v->query);
                Edge *right = NULL;
                auto r1 = fastEval((*cond)[1], &query_marking);
                if (r1 == Condition::RUNKNOWN) {
                    Configuration* c = createConfiguration(v->marking, v->owner, (*cond)[1]);
                    right = newEdge(*v, /*(*cond)[1]->distance(context)*/0);
                    right->addTarget(c);
                } else {
                    bool valid = r1 == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }   // else: right condition is not satisfied, no need to add an edge
                }


                Configuration *left = NULL;
                bool valid = false;
                nextStates(query_marking, cond,
                    [&](){
                        auto r0 = fastEval((*cond)[0], &query_marking);
                        if (r0 == Condition::RUNKNOWN) {
                            left = createConfiguration(v->marking, v->owner, (*cond)[0]);
                        } else {
                            valid = r0 == Condition::RTRUE;
                        }                        
                    },
                    [&](Marking& marking){
                        if(left == NULL && !valid) return false;
                        auto res = fastEval(cond, &marking);
                        if(res == Condition::RFALSE) return true;
                        if(res == Condition::RTRUE)
                        {
                            for(auto s : succs){ --s->refcnt; release(s);}
                            succs.clear();
                            succs.push_back(newEdge(*v, 0));    
                            if(right)
                            {
                                --right->refcnt;
                                release(right);
                                right = nullptr;
                            }
                            
                            if(left)
                            {
                                succs.back()->addTarget(left);                                
                            }
                            
                            return false;
                        }
                        context.setMarking(marking.marking());
                        Edge* e = newEdge(*v, /*cond->distance(context)*/0);
                        Configuration* c1 = createConfiguration(createMarking(marking), owner(marking, cond), cond);
                        e->addTarget(c1);
                        if (left != NULL) {
                            e->addTarget(left);
                        }
                        succs.push_back(e);
                        return true;
                }, [](){});

                if (right != nullptr) {
                    succs.push_back(right);
                }
            }
            else if(v->query->getPath() == F){
                auto cond = static_cast<EFCondition*>(v->query);
                Edge *subquery = NULL;
                auto r = fastEval((*cond)[0], &query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }
                } else {
                    Configuration* c = createConfiguration(v->marking, v->owner, (*cond)[0]);
                    subquery = newEdge(*v, /*cond->distance(context)*/0);
                    subquery->addTarget(c);
                }

                nextStates(query_marking, cond,
                            [](){},
                            [&](Marking& mark){
                                auto res = fastEval(cond, &mark);
                                if(res == Condition::RFALSE) return true;
                                if(res == Condition::RTRUE)
                                {
                                    for(auto s : succs){ --s->refcnt; release(s);}
                                    succs.clear();
                                    succs.push_back(newEdge(*v, 0));
                                    if(subquery)
                                    {
                                        --subquery->refcnt;
                                        release(subquery);
                                    }
                                    subquery = nullptr;
                                    return false;
                                }
                                context.setMarking(mark.marking());
                                Edge* e = newEdge(*v, /*cond->distance(context)*/0);
                                Configuration* c = createConfiguration(createMarking(mark), owner(mark, cond), cond);
                                e->addTarget(c);
                                succs.push_back(e);
                                return true;
                            },
                            [](){}
                        );

                if (subquery != NULL) {
                    succs.push_back(subquery);
                }
            }
            else if(v->query->getPath() == X){
                auto cond = static_cast<EXCondition*>(v->query);
                auto query = (*cond)[0];
                nextStates(query_marking, cond,
                        [](){}, 
                        [&](Marking& marking) {
                            auto res = fastEval(query, &marking);
                            if(res == Condition::RTRUE)
                            {
                                for(auto s : succs){ --s->refcnt; release(s);}
                                succs.clear();
                                succs.push_back(newEdge(*v, 0));
                                return false;
                            }   //else: It can't hold there, no need to create an edge
                            else if(res == Condition::RUNKNOWN)
                            {
                                context.setMarking(marking.marking());
                                Edge* e = newEdge(*v, /*(*cond)[0]->distance(context)*/0);
                                Configuration* c = createConfiguration(createMarking(marking), v->owner, query);
                                e->addTarget(c);
                                succs.push_back(e);
                            }
                            return true;
                        },
                        [](){}
                    );
            }
            else if(v->query->getPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
    }
    else
    {
        assert(false && "Should never happen");
    }
    if(succs.size() == 1 && succs[0]->targets.size() == 1)
    {
        ((PetriConfig*)succs[0]->targets[0])->owner = v->owner;
    }
    return succs;
}

Configuration* OnTheFlyDG::initialConfiguration()
{
    if(working_marking.marking() == nullptr)
    {
        working_marking.setMarking  (net->makeInitialMarking());
        query_marking.setMarking    (net->makeInitialMarking());
        auto o = owner(working_marking, this->query);
        initial_config = createConfiguration(createMarking(working_marking), o, this->query);
    }
    return initial_config;
}


void OnTheFlyDG::nextStates(Marking& t_marking, Condition* ptr,
    std::function<void ()> pre, 
    std::function<bool (Marking&)> foreach, 
    std::function<void ()> post)
{
    bool first = true;
    memcpy(working_marking.marking(), query_marking.marking(), n_places*sizeof(PetriEngine::MarkVal));    
    auto qf = static_cast<QuantifierCondition*>(ptr);
    if(!_partial_order || ptr->getQuantifier() != E || ptr->getPath() != F || (*qf)[0]->isTemporal())
    {
        PetriEngine::SuccessorGenerator PNGen(*net);
        dowork<PetriEngine::SuccessorGenerator>(PNGen, first, pre, foreach);
    }
    else
    {
        _redgen.setQuery(ptr);
        dowork<PetriEngine::ReducingSuccessorGenerator>(_redgen, first, pre, foreach);
    }

    if(!first) post();
}

void OnTheFlyDG::cleanUp()
{    
    while(!recycle.empty())
    {
        assert(recycle.top()->refcnt == -1);
        recycle.pop();
    }
    // TODO, implement proper cleanup
}


void OnTheFlyDG::setQuery(const Condition_ptr& query)
{
    this->query = query;
    delete[] working_marking.marking();
    delete[] query_marking.marking();
    working_marking.setMarking(nullptr);
    query_marking.setMarking(nullptr);
    initialConfiguration();
    assert(this->query);
}

size_t OnTheFlyDG::configurationCount() const
{
    return _configurationCount;
}

size_t OnTheFlyDG::markingCount() const
{
    return _markingCount;
}

PetriConfig *OnTheFlyDG::createConfiguration(size_t marking, size_t own, Condition* t_query)
{
    auto& configs = trie.get_data(marking);
    for(PetriConfig* c : configs){
        if(c->query == t_query)
            return c;
    }

    _configurationCount++;
    size_t id = conf_alloc->next(0);
    char* mem = (*conf_alloc)[id];
    PetriConfig* newConfig = new (mem) PetriConfig();
    newConfig->marking = marking;
    newConfig->query = t_query;
    newConfig->owner = own;
    configs.push_back(newConfig);
    return newConfig;
}



size_t OnTheFlyDG::createMarking(Marking& t_marking){
    size_t sum = 0;
    bool allsame = true;
    uint32_t val = 0;
    uint32_t active = 0;
    uint32_t last = 0;
    markingStats(t_marking.marking(), sum, allsame, val, active, last);
    unsigned char type = encoder.getType(sum, active, allsame, val);
    size_t length = encoder.encode(t_marking.marking(), type);
    binarywrapper_t w = binarywrapper_t(encoder.scratchpad().raw(), length*8);
    auto tit = trie.insert(w.raw(), w.size());
    if(tit.first){
        _markingCount++;
    }

    return tit.second;
}

void OnTheFlyDG::release(Edge* e)
{
    assert(e->refcnt == 0);
    e->is_negated = false;
    e->processed = false;
    e->source = nullptr;
    e->targets.clear();
    e->refcnt = -1;
    e->handled = false;
    recycle.push(e);
}

size_t OnTheFlyDG::owner(Marking& marking, Condition* cond) {
    // Used for distributed algorithm
    return 0;
}


Edge* OnTheFlyDG::newEdge(Configuration &t_source, uint32_t weight)
{
    Edge* e = nullptr;
    if(recycle.empty())
    {
        size_t n = edge_alloc->next(0);
        e = &(*edge_alloc)[n];
    }
    else
    {
        e = recycle.top();
        e->refcnt = 0;
        recycle.pop();
    }
    assert(e->targets.empty());
    e->source = &t_source;
    e->weight = weight;
    assert(e->refcnt == 0);
    ++e->refcnt;
    return e;
}

void OnTheFlyDG::markingStats(const uint32_t* marking, size_t& sum, 
        bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last)
{
    uint32_t cnt = 0;

    for (uint32_t i = 0; i < n_places; i++)
    {
        uint32_t old = val;
        if(marking[i] != 0)
        {
            ++cnt;
            last = std::max(last, i);
            val = std::max(marking[i], val);
            if(old != 0 && marking[i] != old) allsame = false;
            ++active;
            sum += marking[i];
        }
    }
}


}//PetriNet
