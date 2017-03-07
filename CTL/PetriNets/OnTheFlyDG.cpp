#include "OnTheFlyDG.h"

#include "../../PetriEngine/SuccessorGenerator.h"

#include <algorithm>
#include <string.h>
#include <iostream>

using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net){
    net = t_net;
    n_places = t_net->numberOfPlaces();
    n_transitions = t_net->numberOfTransitions();
    initial_marking->setMarking(t_net->makeInitialMarking());
    cached_successors.resize(t_net->numberOfTransitions());

    markings = MarkingContainer(100,                            //Nr. of buckets (bigger maybe?)
                                MarkingEqualityHasher(t_net),     //hasher
                                MarkingEqualityHasher(t_net));    //key_equal
    markings.insert(initial_marking);
}


OnTheFlyDG::~OnTheFlyDG()
{
    cleanUp();
    //Note: initial marking is in the markings set, therefore it will be deleted by the for loop
    //TODO: Ensure we don't leak memory here, when code moving is done
    initial_marking = nullptr;
    for(Marking *m : markings) {
        delete m;
    }
}

bool OnTheFlyDG::fastEval(CTLQuery &query, Marking &marking)
{
    assert(!query.IsTemporal);
    if (query.GetQuantifier() == AND){
        return fastEval(*query.GetFirstChild(), marking) && fastEval(*query.GetSecondChild(), marking);
    } else if (query.GetQuantifier() == OR){
        return fastEval(*query.GetFirstChild(), marking) || fastEval(*query.GetSecondChild(), marking);
    } else if (query.GetQuantifier() == NEG){
        bool result = fastEval(*query.GetFirstChild(), marking);
        return !result;
    } else {
        return evaluateQuery(query, marking);
    }
}


void OnTheFlyDG::successors(Configuration *c)
{
    PetriConfig *v = static_cast<PetriConfig*>(c);
    //    v->printConfiguration();

    CTLType query_type = v->query->GetQueryType();
    if(query_type == EVAL){
        //assert(false && "Someone told me, this was a bad place to be.");
        if (evaluateQuery(*v->query, *v->marking)){
            v->successors.push_back(new Edge(*v));
        }
    }
    else if (query_type == LOPERATOR){
        if(v->query->GetQuantifier() == NEG){
            Configuration* c = createConfiguration(*(v->marking), *(v->query->GetFirstChild()));
            Edge* e = new Edge(*v);
            e->is_negated = true;
            e->targets.push_back(c);
            v->successors.push_back(e);
        }
        else if(v->query->GetQuantifier() == AND){
            //Check if left is false
            if(!v->query->GetFirstChild()->IsTemporal){
                if(!fastEval(*(v->query->GetFirstChild()), *v->marking))
                    //query cannot be satisfied, return empty succ set
                    return;
            }

            //check if right is false
            if(!v->query->GetSecondChild()->IsTemporal){
                if(!fastEval(*(v->query->GetSecondChild()), *v->marking))
                    return;
            }

            Edge *e = new Edge(*v);

            //If we get here, then either both propositions are true (shouldn't be possible)
            //Or a temporal operator and a true proposition
            //Or both are temporal
            if(v->query->GetFirstChild()->IsTemporal){
                e->targets.push_back(createConfiguration(*v->marking, *(v->query->GetFirstChild())));
            }
            if(v->query->GetSecondChild()->IsTemporal){
                e->targets.push_back(createConfiguration(*v->marking, *(v->query->GetSecondChild())));
            }
            v->successors.push_back(e);
        }
        else if(v->query->GetQuantifier() == OR){
            //Check if left is true
            if(!v->query->GetFirstChild()->IsTemporal){
                if(fastEval(*(v->query->GetFirstChild()), *v->marking)){
                    //query is satisfied, return
                    v->successors.push_back(new Edge(*v));
                    return;
                }
            }

            if(!v->query->GetSecondChild()->IsTemporal){
                if(fastEval(*(v->query->GetSecondChild()), *v->marking)){
                    v->successors.push_back(new Edge(*v));
                    return;
                }
            }

            //If we get here, either both propositions are false
            //Or one is false and one is temporal
            //Or both temporal
            if(v->query->GetFirstChild()->IsTemporal){
                Edge *e = new Edge(*v);
                e->targets.push_back(createConfiguration(*v->marking, *(v->query->GetFirstChild())));
                v->successors.push_back(e);
            }
            if(v->query->GetSecondChild()->IsTemporal){
                Edge *e = new Edge(*v);
                e->targets.push_back(createConfiguration(*v->marking, *(v->query->GetSecondChild())));
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
                    bool valid = fastEval(*(v->query->GetSecondChild()), *(v->marking));
                    if (valid) {    //satisfied, no need to go through successors
                        v->successors.push_back(new Edge(*v));
                        return;
                    }//else: It's not valid, no need to add any edge, just add successors
                }
                else {
                    //right side is temporal, we need to evaluate it as normal
                    Configuration* c = createConfiguration(*(v->marking), *(v->query->GetSecondChild()));
                    right = new Edge(*v);
                    right->targets.push_back(c);
                }
                bool valid = false;
                Configuration *left = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    //left side is not temporal, eval it right now!
                    valid = fastEval(*(v->query->GetFirstChild()), *(v->marking));
                } else {
                    //left side is temporal, include it in the edge
                    left = createConfiguration(*(v->marking), *(v->query->GetFirstChild()));
                }
                if (valid || left != NULL) {
                    //if left side is guaranteed to be not satisfied, skip successor generation
                    auto targets = nextState (*(v->marking));

                    if(!targets.empty()){
                        Edge* leftEdge = new Edge(*v);

                        for(auto m : targets){
                            Configuration* c = createConfiguration(*m, *(v->query));
                            leftEdge->targets.push_back(c);
                        }
                        if (left != NULL) {
                            leftEdge->targets.push_back(left);
                        }
                        v->successors.push_back(leftEdge);
                    }
                } //else: Left side is not temporal and it's false, no way to succeed there...

                if (right != NULL) {
                    v->successors.push_back(right);
                }
            }
            else if(v->query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v->query->GetFirstChild()), *(v->marking));
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }
                } else {
                    subquery = new Edge(*v);
                    Configuration* c = createConfiguration(*(v->marking), *(v->query->GetFirstChild()));
                    subquery->targets.push_back(c);
                }

                auto targets = nextState(*(v->marking));

                if(!targets.empty()){
                    Edge* e1 = new Edge(*v);

                    for(auto m : targets){
                        Configuration* c = createConfiguration(*m, *(v->query));
                        e1->targets.push_back(c);
                    }
                    v->successors.push_back(e1);
                }

                if (subquery != NULL) {
                    v->successors.push_back(subquery);
                }
            }
            else if(v->query->GetPath() == X){
                auto targets = nextState(*v->marking);
                if (v->query->GetFirstChild()->IsTemporal) {   //regular check
                    Edge* e = new Edge(*v);
                    for (auto m : targets){
                        Configuration* c = createConfiguration(*m, *(v->query->GetFirstChild()));
                        e->targets.push_back(c);
                    }
                    v->successors.push_back(e);
                } else {
                    bool allValid = true;
                    for (auto m : targets) {
                        bool valid = fastEval(*(v->query->GetFirstChild()), *m);
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
                    Configuration* c = createConfiguration(*(v->marking), *(v->query->GetSecondChild()));
                    right = new Edge(*v);
                    right->targets.push_back(c);
                } else {
                    bool valid = fastEval(*(v->query->GetSecondChild()), *(v->marking));
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }   // else: right condition is not satisfied, no need to add an edge
                }

                auto targets = nextState(*(v->marking));

                if(!targets.empty()){
                    Configuration *left = NULL;
                    bool valid = false;
                    if (v->query->GetFirstChild()->IsTemporal) {
                        left = createConfiguration(*(v->marking), *(v->query->GetFirstChild()));
                    } else {
                        valid = fastEval(*(v->query->GetFirstChild()), *(v->marking));
                    }
                    if (left != NULL || valid) {
                        for(auto m : targets) {
                            Edge* e = new Edge(*v);
                            Configuration* c1 = createConfiguration(*m, *(v->query));
                            e->targets.push_back(c1);
                            if (left != NULL) {
                                e->targets.push_back(left);
                            }
                            v->successors.push_back(e);
                        }
                    }
                }

                if (right != NULL) {
                    v->successors.push_back(right);
                }
            }
            else if(v->query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v->query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v->query->GetFirstChild()), *(v->marking));
                    if (valid) {
                        v->successors.push_back(new Edge(*v));
                        return;
                    }
                } else {
                    Configuration* c = createConfiguration(*(v->marking), *(v->query->GetFirstChild()));
                    subquery = new Edge(*v);
                    subquery->targets.push_back(c);
                }

                auto targets = nextState(*(v->marking));

                if(!targets.empty()){
                    for(auto m : targets){
                        Edge* e = new Edge(*v);
                        Configuration* c = createConfiguration(*m, *(v->query));
                        e->targets.push_back(c);
                        v->successors.push_back(e);
                    }
                }

                if (subquery != NULL) {
                    v->successors.push_back(subquery);
                }
            }
            else if(v->query->GetPath() == X){
                auto targets = nextState(*(v->marking));
                CTLQuery* query = v->query->GetFirstChild();

                if(!targets.empty())
                {
                    if (query->IsTemporal) {    //have to check, no way to skip that
                        for(auto m : targets){
                            Edge* e = new Edge(*v);
                            Configuration* c = createConfiguration(*m, *query);
                            e->targets.push_back(c);
                            v->successors.push_back(e);
                        }
                    } else {
                        for(auto m : targets) {
                            bool valid = fastEval(*query, *m);
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

bool OnTheFlyDG::evaluateQuery(CTLQuery &query, Marking &marking)
{
    assert(query.GetQueryType() == EVAL);
    EvaluateableProposition *proposition = query.GetProposition();

    if (proposition->GetPropositionType() == FIREABILITY) {
        for(const auto f : proposition->GetFireset()){
            // We might be able to optimize this out
            // by keeping track of the fired transitions
            // during successor generation
            if(net->fireable(marking.marking(), f)){
                return true;
            }
        }
        return false;
    }
    else if (proposition->GetPropositionType() == CARDINALITY) {
        int first_param = GetParamValue(proposition->GetFirstParameter(), marking);
        int second_param = GetParamValue(proposition->GetSecondParameter(), marking);
        return EvalCardianlity(first_param, proposition->GetLoperator(), second_param);
    }
    else if (proposition->GetPropositionType() == DEADLOCK) {
        return net->deadlocked(marking.marking());
    }
    else
        assert(false && "Incorrect query proposition type was attempted evaluated");
    exit(EXIT_FAILURE);
}

int OnTheFlyDG::GetParamValue(CardinalityParameter *param, Marking& marking) {
    if(param->isPlace){
        int res = 0;
        for(int place : param->places_i){
            res = res + marking[place];
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
    //initial marking is inserted into the set in the constructor
    Configuration *v = createConfiguration(*initial_marking, *query);
    return v;
}

std::vector<Marking*> OnTheFlyDG::nextState(Marking& t_marking){

    std::vector<Marking*> nextStates;
    PetriEngine::SuccessorGenerator PNGen(*net, t_marking);
    Marking working;
    working.copyMarking(t_marking, net->numberOfPlaces());

    while(PNGen.next(working)){
         nextStates.push_back(createMarking(working));
    }

    return nextStates;
}

void OnTheFlyDG::cleanUp()
{    
    //Note: we clean up the markings in the destructor
    for (Marking *m : markings) {
        for (PetriConfig *c : m->configurations) {
            delete c;
        }
        m->configurations.resize(0);
    }
}

//std::pair<int, int *> OnTheFlyDG::serialize(SearchStrategy::Message &m)
//{
//    //serialized message format: sender | distance | type | id | query_id | marking;
//    PetriConfig *v = static_cast<PetriConfig*>(m.configuration);
//    int size = v->marking->length() + 5;
//    int *buffer = (int*) malloc(sizeof(int) * size);
//    buffer[0] = m.sender;
//    buffer[1] = m.distance;
//    buffer[2] = m.type;
//    buffer[3] = m.id;
//    buffer[4] = v->query->Id;
//    for (int i=0; i < v->marking->length(); i++) {
//        buffer[i+5] = v->marking->value()[i];
//    }
//    return std::pair<int, int*>(size, buffer);
//}

//SearchStrategy::Message OnTheFlyDG::deserialize(int *message, int messageSize)
//{
//    //serialized message format: sender | distance | type | id | query_id | marking;
//    int *markingStart = message + 5;
//    //this constructor will make a copy (that way the message can be safely freed
//    Marking *marking = new Marking(markingStart, messageSize - 5);

//    auto result = markings.find(marking);

//    if(result == markings.end()){
//        marking = *(markings.insert(marking).first);
//    }
//    else{
//        delete marking;
//        marking = *result;
//    }

//    CTLQuery *query = findQueryById(message[4], this->query);
//    assert(query != nullptr);

//    Configuration *c = createConfiguration(*marking, *query);

//    SearchStrategy::Message m(message[0], message[1], (SearchStrategy::Message::Type) message[2], message[3], c);
//    return m;
//}

CTLQuery *OnTheFlyDG::findQueryById(int id, CTLQuery *root)
{
    CTLQuery * result = nullptr;
    if (root->Id == id) {
        result = root;
    } else if (root->GetQueryType() == CTLType::PATHQEURY || root->GetQueryType() == CTLType::LOPERATOR) {
        //we are sure we have the first child
        result = findQueryById(id, root->GetFirstChild());
        if (!(
                    root->GetPath() == Path::F ||
                    root->GetPath() == Path::G ||
                    root->GetPath() == Path::X ||
                    root->GetQuantifier() == Quantifier::NEG
                    ) && result == nullptr) {
            //we are sure we have the second child and we haven't found the query yet
            result = findQueryById(id, root->GetSecondChild());
        }
    }
    return result;
}

void OnTheFlyDG::setQuery(CTLQuery *query)
{
    cleanUp();
    this->query = query;
}

int OnTheFlyDG::configurationCount() const
{
    int count = 0;
    for (Marking *m : markings) {
        count += m->configurations.size();
    }
    return count;
}

int OnTheFlyDG::markingCount() const
{
    return markings.size();
}

Configuration *OnTheFlyDG::createConfiguration(Marking &t_marking, CTLQuery &t_query)
{
    for(PetriConfig* c : t_marking.configurations){
        if(c->query == &t_query)
            return c;
    }

    PetriConfig* newConfig = new PetriConfig(&t_marking, &t_query);
    t_marking.configurations.push_back(newConfig);
    return newConfig;
}



Marking *OnTheFlyDG::createMarking(const Marking& t_marking){
    Marking* new_marking = new Marking();

    new_marking->copyMarking(t_marking, n_places);

    auto result = markings.find(new_marking);

    if(result == markings.end()){
        return *(markings.insert(new_marking).first);
    }
    else{
        delete new_marking;
    }

    return *result;
}

}//PetriNet
