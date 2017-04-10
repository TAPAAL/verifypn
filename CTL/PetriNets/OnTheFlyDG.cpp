#include "OnTheFlyDG.h"

#include "../../PetriEngine/SuccessorGenerator.h"

#include <algorithm>
#include <string.h>
#include <iostream>

using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net) : encoder(t_net->numberOfPlaces(), 0){
    net = t_net;
    n_places = t_net->numberOfPlaces();
    n_transitions = t_net->numberOfTransitions();

    working_marking.setMarking(t_net->makeInitialMarking());
    query_marking.setMarking(t_net->makeInitialMarking());
}


OnTheFlyDG::~OnTheFlyDG()
{
    cleanUp();
    //Note: initial marking is in the markings set, therefore it will be deleted by the for loop
    //TODO: Ensure we don't leak memory here, when code moving is done

}

bool OnTheFlyDG::fastEval(CTLQuery &query, size_t marking, Marking* unfolded)
{
    assert(!query.IsTemporal);
    if (query.GetQuantifier() == AND){
        return fastEval(*query.GetFirstChild(), marking, unfolded) && fastEval(*query.GetSecondChild(), marking, unfolded);
    } else if (query.GetQuantifier() == OR){
        return fastEval(*query.GetFirstChild(), marking, unfolded) || fastEval(*query.GetSecondChild(), marking, unfolded);
    } else if (query.GetQuantifier() == NEG){
        bool result = fastEval(*query.GetFirstChild(), marking, unfolded);
        return !result;
    } else {
        return evaluateQuery(query, marking, unfolded);
    }
}


void OnTheFlyDG::successors(Configuration *c)
{
    PetriConfig *v = static_cast<PetriConfig*>(c);
    trie.unpack(v->marking, encoder.scratchpad().raw());
    encoder.decode(query_marking.marking(), encoder.scratchpad().raw());
    //    v->printConfiguration();

    CTLType query_type = v->query->GetQueryType();
    if(query_type == EVAL){
        //assert(false && "Someone told me, this was a bad place to be.");
        if (evaluateQuery(*v->query, v->marking, &query_marking)){
            v->successors.push_back(new Edge(*v));
        }
    }
    else if (query_type == LOPERATOR){
        if(v->query->GetQuantifier() == NEG){
            Configuration* c = createConfiguration(v->marking, *(v->query->GetFirstChild()));
            Edge* e = new Edge(*v);
            e->is_negated = true;
            e->targets.push_back(c);
            v->successors.push_back(e);
        }
        else if(v->query->GetQuantifier() == AND){
            //Check if left is false
            if(!v->query->GetFirstChild()->IsTemporal){
                if(!fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking))
                    //query cannot be satisfied, return empty succ set
                    return;
            }

            //check if right is false
            if(!v->query->GetSecondChild()->IsTemporal){
                if(!fastEval(*(v->query->GetSecondChild()), v->marking, &query_marking))
                    return;
            }

            Edge *e = new Edge(*v);

            //If we get here, then either both propositions are true (shouldn't be possible)
            //Or a temporal operator and a true proposition
            //Or both are temporal
            if(v->query->GetFirstChild()->IsTemporal){
                e->targets.push_back(createConfiguration(v->marking, *(v->query->GetFirstChild())));
            }
            if(v->query->GetSecondChild()->IsTemporal){
                e->targets.push_back(createConfiguration(v->marking, *(v->query->GetSecondChild())));
            }
            v->successors.push_back(e);
        }
        else if(v->query->GetQuantifier() == OR){
            //Check if left is true
            if(!v->query->GetFirstChild()->IsTemporal){
                if(fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking)){
                    //query is satisfied, return
                    v->successors.push_back(new Edge(*v));
                    return;
                }
            }

            if(!v->query->GetSecondChild()->IsTemporal){
                if(fastEval(*(v->query->GetSecondChild()), v->marking, &query_marking)){
                    v->successors.push_back(new Edge(*v));
                    return;
                }
            }

            //If we get here, either both propositions are false
            //Or one is false and one is temporal
            //Or both temporal
            if(v->query->GetFirstChild()->IsTemporal){
                Edge *e = new Edge(*v);
                e->targets.push_back(createConfiguration(v->marking, *(v->query->GetFirstChild())));
                v->successors.push_back(e);
            }
            if(v->query->GetSecondChild()->IsTemporal){
                Edge *e = new Edge(*v);
                e->targets.push_back(createConfiguration(v->marking, *(v->query->GetSecondChild())));
                v->successors.push_back(e);
            }
        }
        else{
            assert(false && "An unknown error occoured in the loperator-part of the successor generator");
        }
    }
    else if (query_type == PATHQEURY){
        if(v->query->GetQuantifier() == A){
            if (v->query->GetPath() == U){
                Edge *right = NULL;
                if (!v->query->GetSecondChild()->IsTemporal){
                    //right side is not temporal, eval it right now!
                    bool valid = fastEval(*(v->query->GetSecondChild()), v->marking, &query_marking);
                    if (valid) {    //satisfied, no need to go through successors
                        v->successors.push_back(new Edge(*v));
                        return;
                    }//else: It's not valid, no need to add any edge, just add successors
                }
                else {
                    //right side is temporal, we need to evaluate it as normal
                    Configuration* c = createConfiguration(v->marking, *(v->query->GetSecondChild()));
                    right = new Edge(*v);
                    right->targets.push_back(c);
                }
                bool valid = false;
                Configuration *left = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    //left side is not temporal, eval it right now!
                    valid = fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking);
                } else {
                    //left side is temporal, include it in the edge
                    left = createConfiguration(v->marking, *(v->query->GetFirstChild()));
                }
                if (valid || left != NULL) {
                    //if left side is guaranteed to be not satisfied, skip successor generation
                    Edge* leftEdge = NULL;
                    nextStates (query_marking,
                                [&](){ leftEdge = new Edge(*v);},
                                [&](size_t m, Marking&){
                                    Configuration* c = createConfiguration(m, *(v->query));
                                    leftEdge->targets.push_back(c);
                                    return true;
                                },
                                [&]()
                                {
                                    if (left != NULL) {
                                        leftEdge->targets.push_back(left);
                                    }
                                    v->successors.push_back(leftEdge);                                    
                                }
                            );
                } //else: Left side is not temporal and it's false, no way to succeed there...

                if (right != NULL) {
                    v->successors.push_back(right);
                }
            }
            else if(v->query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking);
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }
                } else {
                    subquery = new Edge(*v);
                    Configuration* c = createConfiguration(v->marking, *(v->query->GetFirstChild()));
                    subquery->targets.push_back(c);
                }
                Edge* e1 = NULL;
                nextStates(query_marking,
                        [&](){e1 = new Edge(*v);},
                        [&](size_t m, Marking&)
                        {
                            Configuration* c = createConfiguration(m, *(v->query));
                            e1->targets.push_back(c);
                            return true;
                        },
                        [&]()
                        {
                            v->successors.push_back(e1);
                        }                    
                );

                if (subquery != NULL) {
                    v->successors.push_back(subquery);
                }
            }
            else if(v->query->GetPath() == X){

                if (v->query->GetFirstChild()->IsTemporal) {   //regular check
                    Edge* e = new Edge(*v);
                    nextStates(query_marking, 
                            [](){}, 
                            [&](size_t m, Marking&){
                                Configuration* c = createConfiguration(m, *(v->query->GetFirstChild()));
                                e->targets.push_back(c);
                                return true;
                            }, 
                            [](){}
                        );
                    v->successors.push_back(e);
                } else {
                    auto targets = nextStates(query_marking);
                    bool allValid = true;
                    for (auto m : targets) {
                        bool valid = fastEval(*(v->query->GetFirstChild()), m, NULL);
                        if (!valid) {
                            allValid = false;
                            break;
                        }
                    }
                    if (allValid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }
                }
            }
            else if(v->query->GetPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
        else if(v->query->GetQuantifier() == E){
            if (v->query->GetPath() == U){
                Edge *right = NULL;
                if (v->query->GetSecondChild()->IsTemporal) {
                    Configuration* c = createConfiguration(v->marking, *(v->query->GetSecondChild()));
                    right = new Edge(*v);
                    right->targets.push_back(c);
                } else {
                    bool valid = fastEval(*(v->query->GetSecondChild()), v->marking, &query_marking);
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }   // else: right condition is not satisfied, no need to add an edge
                }


                Configuration *left = NULL;
                bool valid = false;
                nextStates(query_marking, 
                    [&](){
                        if (v->query->GetFirstChild()->IsTemporal) {
                            left = createConfiguration(v->marking, *(v->query->GetFirstChild()));
                        } else {
                            valid = fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking);
                        }                        
                    },
                    [&](size_t m, Marking&){
                        if(left == NULL && !valid) return false;
                        Edge* e = new Edge(*v);
                        Configuration* c1 = createConfiguration(m, *(v->query));
                        e->targets.push_back(c1);
                        if (left != NULL) {
                            e->targets.push_back(left);
                        }
                        v->successors.push_back(e);
                        return true;
                }, [](){});

                if (right != NULL) {
                    v->successors.push_back(right);
                }
            }
            else if(v->query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v->query->GetFirstChild()), v->marking, &query_marking);
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }
                } else {
                    Configuration* c = createConfiguration(v->marking, *(v->query->GetFirstChild()));
                    subquery = new Edge(*v);
                    subquery->targets.push_back(c);
                }

                nextStates(query_marking,
                            [](){},
                            [&](size_t m, Marking&){
                                Edge* e = new Edge(*v);
                                Configuration* c = createConfiguration(m, *(v->query));
                                e->targets.push_back(c);
                                v->successors.push_back(e);
                                return true;
                            },
                            [](){}
                        );

                if (subquery != NULL) {
                    v->successors.push_back(subquery);
                }
            }
            else if(v->query->GetPath() == X){
                auto targets = nextStates(query_marking);
                CTLQuery* query = v->query->GetFirstChild();

                if(!targets.empty())
                {
                    if (query->IsTemporal) {    //have to check, no way to skip that
                        for(auto m : targets){
                            Edge* e = new Edge(*v);
                            Configuration* c = createConfiguration(m, *query);
                            e->targets.push_back(c);
                            v->successors.push_back(e);
                        }
                    } else {
                        for(auto m : targets) {
                            bool valid = fastEval(*query, m, NULL);
                            if (valid) {
                                v->successors.push_back(new Edge(*v));
                                return;
                            }   //else: It can't hold there, no need to create an edge
                        }
                    }
                }
            }
            else if(v->query->GetPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
    }
}

bool OnTheFlyDG::evaluateQuery(CTLQuery &query, size_t marking, Marking* unfolded)
{
    if(unfolded == NULL)
    {
        trie.unpack(marking, encoder.scratchpad().raw());
        encoder.decode(working_marking.marking(), encoder.scratchpad().raw());
        unfolded = &working_marking;
    }
    
    assert(query.GetQueryType() == EVAL);
    EvaluateableProposition *proposition = query.GetProposition();

    if (proposition->GetPropositionType() == FIREABILITY) {
        for(const auto f : proposition->GetFireset()){
            // We might be able to optimize this out
            // by keeping track of the fired transitions
            // during successor generation
            if(net->fireable(unfolded->marking(), f)){
                return true;
            }
        }
        return false;
    }
    else if (proposition->GetPropositionType() == CARDINALITY) {
        int first_param = GetParamValue(proposition->GetFirstParameter(), *unfolded);
        int second_param = GetParamValue(proposition->GetSecondParameter(), *unfolded);
        return EvalCardianlity(first_param, proposition->GetLoperator(), second_param);
    }
    else if (proposition->GetPropositionType() == DEADLOCK) {
        return net->deadlocked(unfolded->marking());
    }
    else
        assert(false && "Incorrect query proposition type was attempted evaluated");
    exit(EXIT_FAILURE);
}

int OnTheFlyDG::GetParamValue(CardinalityParameter *param, Marking& marking) {
    if(param->isPlace){
        int res = 0;
        for(int place : param->places_i){
            res = res + marking.marking()[place];
        }
        return res;
    }
    else{
        return param->value;
    }
}

bool OnTheFlyDG::EvalCardianlity(int a, LoperatorType lop, int b) {
    if(lop == EQ)
        return a == b;
    else if (lop == LE)
        return a < b;
    else if (lop == LEQ)
        return a <= b;
    else if (lop == GR)
        return a > b;
    else if (lop == GRQ)
        return a >= b;
    else if (lop == NE)
        return a != b;
    std::cerr << "Error: Query unsupported - Attempted to compare " << a << " with " << b << " by an unknown logical operator " << std::endl;
    exit(EXIT_FAILURE);
}


Configuration* OnTheFlyDG::initialConfiguration()
{
    return initial_config;
}


std::vector<size_t> OnTheFlyDG::nextStates(Marking& t_marking)
{
    std::vector<size_t> succs;
    nextStates(t_marking, 
            [](){},
            [&](size_t n, Marking& m){succs.push_back(n); return true;},
            [](){});
    return succs;
}
void OnTheFlyDG::nextStates(Marking& t_marking, 
    std::function<void ()> pre, 
    std::function<bool (size_t, Marking&)> foreach, 
    std::function<void ()> post)
{

    bool first = true;
    PetriEngine::SuccessorGenerator PNGen(*net);
    PNGen.prepare(&query_marking);
    memcpy(working_marking.marking(), query_marking.marking(), n_places*sizeof(PetriEngine::MarkVal));    

    
    while(PNGen.next(working_marking)){
        if(first) pre();
        first = false;
        if(!foreach(createMarking(working_marking), working_marking))
        {
            break;
        }
    }

    if(!first) post();
}

void OnTheFlyDG::cleanUp()
{    
    // TODO, implement proper cleanup
}

void OnTheFlyDG::setQuery(CTLQuery *query)
{
    cleanUp();
    this->query = query;
    initial_config = createConfiguration(createMarking(working_marking), *query);
}

int OnTheFlyDG::configurationCount() const
{
    return _configurationCount;
}

int OnTheFlyDG::markingCount() const
{
    return _markingCount;
}

PetriConfig *OnTheFlyDG::createConfiguration(size_t t_marking, CTLQuery &t_query)
{
    auto& configs = trie.getData(t_marking);
    for(PetriConfig* c : configs){
        if(c->query == &t_query)
            return c;
    }

    _configurationCount++;
    PetriConfig* newConfig = new PetriConfig(t_marking, &t_query);
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
    auto tit = trie.insert(w);
    if(tit.first){
        _markingCount++;
    }
	return tit.second;
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
