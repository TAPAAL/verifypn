#include "CTL/PetriNets/OnTheFlyDG.h"

#include <algorithm>
#include <string.h>
#include <iostream>
#include <queue>
#include <limits>

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/PQL/Expressions.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/Evaluation.h"

using namespace PetriEngine::PQL;
using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net, bool partial_order, TokenEliminator& token_elim) : encoder(t_net->numberOfPlaces(), 0),
        token_elim(token_elim),
        edge_alloc(new linked_bucket_t<DependencyGraph::Edge,1024*10>(1)),
        conf_alloc(new linked_bucket_t<char[sizeof(PetriConfig)], 1024*1024>(1)),
        _redgen(*t_net, std::make_shared<PetriEngine::ReachabilityStubbornSet>(*t_net)), _partial_order(partial_order) {
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
    return PetriEngine::PQL::evaluate(query, e);
}

Condition::Result OnTheFlyDG::fastEval(Condition* query, Marking* unfolded)
{
    EvaluationContext e(unfolded->marking(), net);
    return PetriEngine::PQL::evaluate(query, e);
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
            succs.push_back(newEdge(*v, 0));///*v->query->distance(context))*/0);F
        }
    }
    else if (query_type == LOPERATOR){
        if(v->query->getQuantifier() == NEG){
            // no need to try to evaluate here -- this is already transient in other evaluations.
            auto cond = static_cast<NotCondition*>(v->query);
            Configuration* c = createConfiguration(query_marking, v->getOwner(), (*cond)[0]);
            Edge* e = newEdge(*v, /*v->query->distance(context)*/0);
            e->is_negated = true;
            if (!e->addTarget(c)) {
                succs.push_back(e);
            }
            else {
                --e->refcnt;
                release(e);
            }
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
                assert(PetriEngine::PQL::isTemporal(c));
                if (e->addTarget(createConfiguration(query_marking, v->getOwner(), c)))
                    break;
            }
            if (e->handled) {
                --e->refcnt;
                release(e);
            }
            else
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
                assert(PetriEngine::PQL::isTemporal(c));
                Edge *e = newEdge(*v, /*cond->distance(context)*/0);
                if (e->addTarget(createConfiguration(query_marking, v->getOwner(), c))) {
                    --e->refcnt;
                    release(e);
                }
                else
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
                Edge *right = nullptr;
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
                    Configuration* c = createConfiguration(query_marking, v->getOwner(), (*cond)[1]);
                    right = newEdge(*v, /*(*cond)[1]->distance(context)*/0);
                    right->addTarget(c);
                }
                bool valid = false;
                Configuration *left = nullptr;
                auto r0 = fastEval((*cond)[0], &query_marking);
                if (r0 != Condition::RUNKNOWN) {
                    //left side is not temporal, eval it right now!
                    valid = r0 == Condition::RTRUE;
                } else {
                    //left side is temporal, include it in the edge
                    left = createConfiguration(query_marking, v->getOwner(), (*cond)[0]);
                }
                if (valid || left != nullptr) {
                    //if left side is guaranteed to be not satisfied, skip successor generation
                    Edge* leftEdge = nullptr;
                    nextStates (query_marking, cond,
                                [&](){ leftEdge = newEdge(*v, std::numeric_limits<uint32_t>::max());},
                                [&](Marking& mark){
                                    auto res = fastEval(cond, &mark);
                                    if(res == Condition::RTRUE) return true;
                                    if(res == Condition::RFALSE)
                                    {
                                        left = nullptr;
                                        --leftEdge->refcnt;
                                        release(leftEdge);
                                        leftEdge = nullptr;
                                        return false;
                                    }
                                    context.setMarking(mark.marking());
                                    Configuration* c = createConfiguration(mark, owner(mark, cond), cond);
                                    return !leftEdge->addTarget(c);
                                },
                                [&]()
                                {
                                    if(leftEdge)
                                    {
                                        if (left != nullptr) {
                                            leftEdge->addTarget(left);
                                        }
                                        if (leftEdge->handled){
                                            --leftEdge->refcnt;
                                            release(leftEdge);
                                            leftEdge = nullptr;
                                        }
                                        else
                                            succs.push_back(leftEdge);
                                    }
                                }
                            );
                } //else: Left side is not temporal and it's false, no way to succeed there...

                if (right != nullptr) {
                    if (right->handled){
                        --right->refcnt;
                        release(right);
                    }
                    else
                        succs.push_back(right);
                }
            }
            else if(v->query->getPath() == F){
                auto cond = static_cast<AFCondition*>(v->query);
                Edge *subquery = nullptr;
                auto r = fastEval((*cond)[0], &query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }
                } else {
                    subquery = newEdge(*v, /*cond->distance(context)*/0);
                    Configuration* c = createConfiguration(query_marking, v->getOwner(), (*cond)[0]);
                    subquery->addTarget(c); // cannot be self-loop since the formula is smaller
                }
                Edge* e1 = nullptr;
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
                            Configuration* c = createConfiguration(mark, owner(mark, cond), cond);
                            return !e1->addTarget(c);
                        },
                        [&]()
                        {
                            if (e1->handled) {
                                --e1->refcnt;
                                release(e1);
                            }
                            else
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
                // no possible self-loops from AX q
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
                                Configuration* c = createConfiguration(mark, v->getOwner(), (*cond)[0]);
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
                Edge *right = nullptr;
                auto r1 = fastEval((*cond)[1], &query_marking);
                if (r1 == Condition::RUNKNOWN) {
                    Configuration* c = createConfiguration(query_marking, v->getOwner(), (*cond)[1]);
                    right = newEdge(*v, /*(*cond)[1]->distance(context)*/0);
                    right->addTarget(c);
                } else {
                    bool valid = r1 == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }   // else: right condition is not satisfied, no need to add an edge
                }


                Configuration *left = nullptr;
                bool valid = false;
                nextStates(query_marking, cond,
                    [&](){
                        auto r0 = fastEval((*cond)[0], &query_marking);
                        if (r0 == Condition::RUNKNOWN) {
                            left = createConfiguration(query_marking, v->getOwner(), (*cond)[0]);
                        } else {
                            valid = r0 == Condition::RTRUE;
                        }
                    },
                    [&](Marking& marking){
                        if(left == nullptr && !valid) return false;
                        auto res = fastEval(cond, &marking);
                        if(res == Condition::RFALSE) return true;
                        if(res == Condition::RTRUE)
                        {
                            for(auto s : succs){ --s->refcnt; release(s);}
                            succs.clear();
                            succs.push_back(newEdge(*v, 0));
                            if(right && (left == nullptr && valid))
                            {
                                // we don't need to validate right IF left
                                // is trivially satisfied and we have a satisfied
                                // successor.
                                --right->refcnt;
                                release(right);
                                right = nullptr;
                            }

                            if(left)
                                succs.back()->addTarget(left);

                            return false;
                        }
                        context.setMarking(marking.marking());
                        Edge* e = newEdge(*v, /*cond->distance(context)*/0);
                        Configuration* c1 = createConfiguration(marking, owner(marking, cond), cond);
                        e->addTarget(c1);
                        if (left != nullptr) {
                            e->addTarget(left);
                        }
                        if (e->handled) {
                            --e->refcnt;
                            release(e);
                            // we _don't_ abort suc generation, since EU will have many out-edges
                        }
                        else
                            succs.push_back(e);
                        return true;
                }, [](){});

                if (right != nullptr) {
                    if (right->handled) {
                        --right->refcnt;
                        release(right);
                    }
                    else
                        succs.push_back(right);
                }
            }
            else if(v->query->getPath() == F){
                auto cond = static_cast<EFCondition*>(v->query);
                Edge *subquery = nullptr;
                auto r = fastEval((*cond)[0], &query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(newEdge(*v, 0));
                        return succs;
                    }
                } else {
                    Configuration* c = createConfiguration(query_marking, v->getOwner(), (*cond)[0]);
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
                                Configuration* c = createConfiguration(mark, owner(mark, cond), cond);
                                e->addTarget(c);
                                if (!e->handled)
                                    succs.push_back(e);
                                else {
                                    --e->refcnt;
                                    release(e);
                                }
                                return true;
                            },
                            [](){}
                        );

                if (subquery != nullptr) {
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
                                Configuration* c = createConfiguration(marking, v->getOwner(), query);
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
    /*
    // legacy code from distributed version
    if(succs.size() == 1 && succs[0]->targets.size() == 1)
    {
        ((PetriConfig*)succs[0]->targets[0])->setOwner(v->getOwner());
    }*/
    return succs;
}

Configuration* OnTheFlyDG::initialConfiguration()
{
    if(working_marking.marking() == nullptr)
    {
        working_marking.setMarking(net->makeInitialMarking());
        query_marking.setMarking(net->makeInitialMarking());
        abstracted_marking.setMarking(net->makeInitialMarking());
        auto o = owner(working_marking, this->query);
        initial_config = createConfiguration(working_marking, o, this->query);
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
    if(!_partial_order || ptr->getQuantifier() != E || ptr->getPath() != F || PetriEngine::PQL::isTemporal((*qf)[0]))
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
    if (std::getenv("CZERO_CONF_DEBUG") != nullptr)
    {
        for (auto& c : trie)
        {
            for(auto& conf : c)
            {
                trie.unpack(conf->marking, encoder.scratchpad().raw());
                encoder.decode(query_marking.marking(), encoder.scratchpad().raw());

                std::cout << "CONF M:";
                for (uint32_t i = 0; i < n_places; i++)
                {
                    std::cout << query_marking[i];
                }
                std::cout << " Q:";
                conf->query->toString(std::cout);
                char ass = "0?%1"[conf->assignment + 2];
                std::cout << " A:" << ass << std::endl;
            }
        }
    }

    while(!recycle.empty())
    {
        assert(recycle.top()->refcnt == -1);
        recycle.pop();
    }
    // TODO, implement proper cleanup
}


void OnTheFlyDG::setQuery(Condition* query)
{
    this->query = query;
    delete[] working_marking.marking();
    delete[] query_marking.marking();
    delete[] abstracted_marking.marking();
    working_marking.setMarking(nullptr);
    query_marking.setMarking(nullptr);
    abstracted_marking.setMarking(nullptr);
    token_elim.init(net);
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

size_t OnTheFlyDG::maxTokens() const {
    return _maxTokens;
}

size_t OnTheFlyDG::tokensEliminated() const {
    return token_elim.tokensEliminated();
}

PetriConfig *OnTheFlyDG::createConfiguration(const Marking& marking, size_t own, Condition* t_query)
{
    abstracted_marking.copy(marking.marking(), n_places);
    token_elim.eliminate(&abstracted_marking, t_query);

    size_t encoded = createMarking(abstracted_marking);
    auto& configs = trie.get_data(encoded);
    for(PetriConfig* c : configs){
        if(c->query == t_query)
            return c;
    }

    _configurationCount++;
    size_t id = conf_alloc->next(0);
    char* mem = (*conf_alloc)[id];
    PetriConfig* newConfig = new (mem) PetriConfig();
    newConfig->marking = encoded;
    newConfig->query = t_query;
    newConfig->setOwner(own);
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
        _maxTokens = std::max(sum, _maxTokens);
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
    /*e->assignment = UNKNOWN;
    e->children = 0;*/
    e->source = &t_source;
    assert(e->refcnt == 0);
    assert(!e->handled);
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
