/* 
 * File:   Reducer.cpp
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#include "Reducer.h"
#include "PetriNet.h"
#include "PetriNetBuilder.h"
#include <PetriParse/PNMLParser.h>
#include <queue>
#include <set>
#include <algorithm>

namespace PetriEngine {

    Reducer::Reducer(PetriNetBuilder* p) 
    : parent(p) {
    }

    Reducer::~Reducer() {

    }

    void Reducer::Print(QueryPlaceAnalysisContext& context) {
        std::cout   << "\nNET INFO:\n" 
                    << "Number of places: " << parent->numberOfPlaces() << std::endl
                    << "Number of transitions: " << parent->numberOfTransitions() 
                    << std::endl << std::endl;
        for (uint32_t t = 0; t < parent->numberOfTransitions(); t++) {
            std::cout << "Transition " << t << " :\n";
            if(parent->_transitions[t].skip)
            {
                std::cout << "\tSKIPPED" << std::endl;
            }
            for(auto& arc : parent->_transitions[t].pre)
            {
                if (arc.weight > 0) 
                    std::cout   << "\tInput place " << arc.place  
                                << " (" << getPlaceName(arc.place) << ")"
                                << " with arc-weight " << arc.weight << std::endl;
            }
            for(auto& arc : parent->_transitions[t].post)
            {
                if (arc.weight > 0) 
                    std::cout   << "\tOutput place " << arc.place 
                                << " (" << getPlaceName(arc.place) << ")" 
                                << " with arc-weight " << arc.weight << std::endl;
            }
            std::cout << std::endl;
        }
        for (uint32_t i = 0; i < parent->numberOfPlaces(); i++) {
            std::cout <<    "Marking at place "<< i << 
                            " is: " << parent->initMarking()[i] << std::endl;
        }
        for (uint32_t i = 0; i < parent->numberOfPlaces(); i++) {
            std::cout   << "Query count for place " << i 
                        << " is: " << context.getQueryPlaceCount()[i] << std::endl;
        }
    }
    
    std::string Reducer::getTransitionName(uint32_t transition)
    {
        for(auto t : parent->_transitionnames)
        {
            if(t.second == transition) return t.first;
        }
        assert(false);
        return "";
    }
    
    std::string Reducer::getPlaceName(uint32_t place)
    {
        for(auto t : parent->_placenames)
        {
            if(t.second == place) return t.first;
        }
        assert(false);
        return "";
    }
    
    Transition& Reducer::getTransition(uint32_t transition)
    {
        return parent->_transitions[transition];
    }
    
    ArcIter Reducer::getOutArc(Transition& trans, uint32_t place)
    {
        Arc a;
        a.place = place;
        auto ait = std::lower_bound(trans.post.begin(), trans.post.end(), a);
        if(ait != trans.post.end() && ait->place == place)
        {
            return ait;
        }
        else 
        {
            return trans.post.end();
        }
    }
    
    ArcIter Reducer::getInArc(uint32_t place, Transition& trans)
    {
        Arc a;
        a.place = place;
        auto ait = std::lower_bound(trans.pre.begin(), trans.pre.end(), a);
        if(ait != trans.pre.end() && ait->place == place)
        {
            return ait;
        }
        else 
        {
            return trans.pre.end();
        }
    }
    
    void Reducer::eraseTransition(std::vector<uint32_t>& set, uint32_t el)
    {
        auto lb = std::lower_bound(set.begin(), set.end(), el);
        assert(lb != set.end());
        assert(*lb == el);
        set.erase(lb);
    }
    
    void Reducer::skipTransition(uint32_t t)
    {
        ++_removedTransitions;
        Transition& trans = getTransition(t);
        assert(!trans.skip);
        for(auto p : trans.post)
        {
            eraseTransition(parent->_places[p.place].producers, t);
        }
        for(auto p : trans.pre)
        {
            eraseTransition(parent->_places[p.place].consumers, t);
        }
        trans.post.clear();
        trans.pre.clear();
        trans.skip = true;
        assert(consistent());
    }
    
    void Reducer::skipPlace(uint32_t place)
    {
        ++_removedPlaces;
        Place& pl = parent->_places[place];
        assert(!pl.skip);
        pl.skip = true;
        for(auto& t : pl.consumers)
        {
            Transition& trans = getTransition(t);
            auto ait = getInArc(place, trans);
            if(ait != trans.pre.end() && ait->place == place)
                trans.pre.erase(ait);
        }

        for(auto& t : pl.producers)
        {
            Transition& trans = getTransition(t);
            auto ait = getOutArc(trans, place);
            if(ait != trans.post.end() && ait->place == place)
                trans.post.erase(ait);
        }
        pl.consumers.clear();
        pl.producers.clear();
        assert(consistent());
    }
    
    
    bool Reducer::consistent()
    {
#ifndef NDEBUG
        size_t strans = 0;
        for(size_t i = 0; i < parent->numberOfTransitions(); ++i)
        {
            Transition& t = parent->_transitions[i];
            if(t.skip) ++strans;
            assert(std::is_sorted(t.pre.begin(), t.pre.end()));
            assert(std::is_sorted(t.post.end(), t.post.end()));
            assert(!t.skip || (t.pre.size() == 0 && t.post.size() == 0));
            for(Arc& a : t.pre)
            {
                assert(a.weight > 0);
                Place& p = parent->_places[a.place];
                assert(!p.skip);
                assert(std::find(p.consumers.begin(), p.consumers.end(), i) != p.consumers.end());
            }
            for(Arc& a : t.post)
            {
                assert(a.weight > 0);
                Place& p = parent->_places[a.place];
                assert(!p.skip);
                assert(std::find(p.producers.begin(), p.producers.end(), i) != p.producers.end());
            }
        }
        
        assert(strans == _removedTransitions);

        size_t splaces = 0;
        for(size_t i = 0; i < parent->numberOfPlaces(); ++i)
        {
            Place& p = parent->_places[i];
            if(p.skip) ++splaces;
            assert(std::is_sorted(p.consumers.begin(), p.consumers.end()));
            assert(std::is_sorted(p.producers.begin(), p.producers.end()));
            assert(!p.skip || (p.consumers.size() == 0 && p.producers.size() == 0));
            
            for(uint c : p.consumers)
            {
                Transition& t = parent->_transitions[c];
                assert(!t.skip);
                auto a = getInArc(i, t);
                assert(a != t.pre.end());
                assert(a->place == i);
            }
            
            for(uint prod : p.producers)
            {
                Transition& t = parent->_transitions[prod];
                assert(!t.skip);
                auto a = getOutArc(t, i);
                assert(a != t.post.end());
                assert(a->place == i);
            }
        }
        assert(splaces == _removedPlaces);
#endif
        return true;
    }

    bool Reducer::ReducebyRuleA(uint32_t* placeInQuery) {
        // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places (and t)  
        bool continueReductions = false;
        const size_t numberoftransitions = parent->numberOfTransitions();
        for (uint32_t t = 0; t < numberoftransitions; t++) {
            if(hasTimedout()) return false;
            Transition& trans = getTransition(t);
                        
            // we have already removed
            if(trans.skip) continue;

            // A2. we have more/less than one arc in pre or post
            // checked first to avoid out-of-bounds when looking up indexes.
            if(trans.pre.size() != 1) continue;

            uint32_t pPre = trans.pre[0].place;
                        
            // A2. Check that pPre goes only to t
            if(parent->_places[pPre].consumers.size() != 1) continue;
            
            // A3. We have weight of more than one on input
            // and is empty on output (should not happen).
            if(trans.pre[0].weight != 1) continue;
                        
            // A4. Do inhibitor check, neither T, pPre or pPost can be involved with any inhibitor
            if(parent->_places[pPre].inhib|| trans.inhib) continue;
            
            // A5. dont mess with query!
            if(placeInQuery[pPre] > 0) continue;
            // check A1, A4 and A5 for post
            bool ok = true;
            for(auto& pPost : trans.post)
            {
                if(parent->_places[pPost.place].inhib || pPre == pPost.place || placeInQuery[pPost.place] > 0)
                {
                    ok = false;
                    break;
                }
            }
            if(!ok) continue;
            
            continueReductions = true;
            _ruleA++;
            
            // here we need to remember when a token is created in pPre (some 
            // transition with an output in P is fired), t is fired instantly!.
            if(reconstructTrace) {
                Place& pre = parent->_places[pPre];
                std::string tname = getTransitionName(t);
                for(size_t pp : pre.producers)
                {
                    std::string prefire = getTransitionName(pp);
                    _postfire[prefire].push_back(tname);
                }
                _extraconsume[tname].emplace_back(getPlaceName(pPre), trans.pre[0].weight);
                for(size_t i = 0; i < parent->initMarking()[pPre]; ++i)
                {
                    _initfire.push_back(tname);
                }                
            }
            
            for(auto& pPost : trans.post)
            {
                // UA2. move the token for the initial marking, makes things simpler.
                parent->initialMarking[pPost.place] += (parent->initialMarking[pPre] * pPost.weight);
            }
            parent->initialMarking[pPre] = 0;

            // Remove transition t and the place that has no tokens in m0
            // UA1. remove transition
            auto toMove = trans.post;
            skipTransition(t);

            // UA2. update arcs
            for(auto& _t : parent->_places[pPre].producers)
            {
                assert(_t != t);
                // move output-arcs to post.
                Transition& src = getTransition(_t);
                auto source = *getOutArc(src, pPre);
                for(auto& pPost : toMove)
                {
                    Arc a;
                    a.place = pPost.place;
                    a.weight = source.weight * pPost.weight;
                    assert(a.weight > 0);
                    a.inhib = false;
                    auto dest = std::lower_bound(src.post.begin(), src.post.end(), a);
                    if(dest == src.post.end() || dest->place != pPost.place)
                    {
                        dest = src.post.insert(dest, a);
                        auto& prod = parent->_places[pPost.place].producers;
                        auto lb = std::lower_bound(prod.begin(), prod.end(), _t);
                        prod.insert(lb, _t);
                    }
                    else
                    {
                        dest->weight += (source.weight * pPost.weight);
                    }
                    assert(dest->weight > 0);
                }
            }                
            // UA1. remove place
            skipPlace(pPre);
        } // end of Rule A main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleB(uint32_t* placeInQuery) {
        // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();
        for (uint32_t p = 0; p < numberofplaces; p++) {
            if(hasTimedout()) return false;            
            Place& place = parent->_places[p];
            
            if(place.skip) continue;    // already removed    
            
            // B5. dont mess up query
            if(placeInQuery[p] > 0) continue;
                        
            // B2. Only one consumer/producer
            if( place.consumers.size() != 1 || 
                place.producers.size() != 1) continue; // no orphan removal
            
            int tOut = place.producers[0];
            int tIn = place.consumers[0];
            
            // B1. producer is not consumer
            if (tOut == tIn) continue; // cannot remove this kind either
                        
            Transition& out = getTransition(tOut);
            Transition& in = getTransition(tIn);
            
            auto inArc = getInArc(p, in);
            auto outArc = getOutArc(out, p);
            
            // B3. Output is a multiple of input and nonzero.
            if(outArc->weight < inArc->weight) continue;            
            if((outArc->weight % inArc->weight) != 0) continue;
            
            size_t multiplier = outArc->weight / inArc->weight;
            
            // B4. Do inhibitor check, neither In, out or place can be involved with any inhibitor
            if(place.inhib || in.inhib || out.inhib) continue;
            
            // B6. also, none of the places in the post-set of consuming transition can be participating in inhibitors.
            // B7. nor can they appear in the query.
            {
                bool post_ok = false;
                for(const Arc& a : in.post)
                {
                    post_ok |= parent->_places[a.place].inhib;
                    post_ok |= placeInQuery[a.place];
                    if(post_ok) break;
                }
                if(post_ok) continue;
            }

            bool ok = true;
            // B2. Check that there is no other place than p that gives to tPost, 
            // tPre can give to other places
            for (auto& arc : in.pre) {
                if (arc.weight > 0 && arc.place != p) {
                    ok = false;
                    break;
                }
            }
            
            if (!ok) {
                continue;
            }

            // UB2. we need to remember initial marking
            uint initm = parent->initMarking()[p];
            initm /= inArc->weight; // integer-devision is floor by default

            if(reconstructTrace)
            {
                // remember reduction for recreation of trace
                std::string toutname    = getTransitionName(tOut);
                std::string tinname     = getTransitionName(tIn);
                std::string pname       = getPlaceName(p);
                Arc& a = *getInArc(p, in);
                _extraconsume[tinname].emplace_back(pname, a.weight);
                for(size_t i = 0; i < multiplier; ++i)
                {
                    _postfire[toutname].push_back(tinname);
                }
                
                for(size_t i = 0; initm > 0 && i < initm / inArc->weight; ++i )
                {
                    _initfire.push_back(tinname);
                }
            }
            
            continueReductions = true;
            _ruleB++;
             // UB1. Remove place p
            skipPlace(p);
            parent->initialMarking[p] = 0;
            // We need to remember that when tOut fires, tIn fires just after.
            // this should fix the trace

            // UB3. move arcs from t' to t
            for (auto& arc : in.post) { // remove tPost
                auto _arc = getOutArc(out, arc.place);
                // UB2. Update initial marking
                parent->initialMarking[arc.place] += initm*arc.weight;
                if(_arc != out.post.end())
                {
                    _arc->weight += arc.weight*multiplier;
                }
                else
                {
                    out.post.push_back(arc);
                    out.post.back().weight *= multiplier;
                    parent->_places[arc.place].producers.push_back(tOut);
                    
                    std::sort(out.post.begin(), out.post.end());
                    std::sort(parent->_places[arc.place].producers.begin(),
                              parent->_places[arc.place].producers.end());
                }
            }
            // UB1. remove transition
            skipTransition(tIn);
        } // end of Rule B main for-loop
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleC(uint32_t* placeInQuery) {
        // Rule C - Places with same input and output-transitions which a modulo each other
        bool continueReductions = false;
        
        const size_t numberofplaces = parent->numberOfPlaces();
        
        for(uint32_t pouter = 0; pouter < numberofplaces; ++pouter)
        {                        
            if(hasTimedout()) return false;
            if(parent->_places[pouter].skip) continue;
            
            // C4. No inhib
            if(parent->_places[pouter].inhib) continue;
            
            for (uint32_t pinner = pouter + 1; pinner < numberofplaces; ++pinner) 
            {
                if(parent->_places[pinner].skip) continue;

                // C4. No inhib
                if(parent->_places[pinner].inhib) continue;

                for(size_t swp = 0; swp < 2; ++swp)
                {
                    if( parent->_places[pinner].skip ||
                        parent->_places[pouter].skip) break;
                    
                    uint p1 = pouter;
                    uint p2 = pinner;
                    
                    if(swp == 1) std::swap(p1, p2);
                    
                    Place& place1 = parent->_places[p1];

                    // C1. Not same place
                    if(p1 == p2) break;

                    // C5. Dont mess with query
                    if(placeInQuery[p2] > 0)
                        continue;

                    Place& place2 = parent->_places[p2];

                    // C2, C3. Consumer and producer-sets must match
                    if(place1.consumers.size() < place2.consumers.size() ||
                       place1.producers.size() > place2.producers.size())
                        break;

                    double mult = std::numeric_limits<double>::infinity();

                    // C8. Consumers must match with weights
                    int ok = 0;
                    size_t j = 0;
                    for(size_t i = 0; i < place2.consumers.size(); ++i)
                    {
                        while(place1.consumers[j] < place2.consumers[i] && j < place1.consumers.size()) ++j;
                        if(place1.consumers.size() <= j || place1.consumers[j] != place2.consumers[i])
                        {
                            ok = 2;
                            break;
                        }

                        Transition& trans = getTransition(place1.consumers[j]);
                        auto a1 = getInArc(p1, trans);
                        auto a2 = getInArc(p2, trans);
                        assert(a1 != trans.pre.end());
                        assert(a2 != trans.pre.end());
                        mult = std::min(mult, ((double)a2->weight) / ((double)a1->weight));

                        if(mult < 1)
                        {
                            ok = 1;
                            break;
                        }
                    }

                    if(ok == 2) break;
                    else if(ok == 1) continue;

                    // C6. We do not care about excess markings in p2.
                    if(mult != std::numeric_limits<double>::infinity() &&
                            (((double)parent->initialMarking[p1]) * mult) > (double)parent->initialMarking[p2])
                    {
                        continue;
                    }

                    
                    // C7. Producers must match with weights
                    j = 0;
                    for(size_t i = 0; i < place1.producers.size(); ++i)
                    {
                        while(place2.producers[j] < place1.producers[i] && j < place2.producers.size()) ++j;
                        if(place1.producers[i] != place2.producers[j])
                        {
                            ok = 2;
                            break;
                        }

                        Transition& trans = getTransition(place1.producers[i]);
                        auto a1 = getOutArc(trans, p1);
                        auto a2 = getOutArc(trans, p2);
                        assert(a1 != trans.post.end());
                        assert(a2 != trans.post.end());

                        if(((double)a1->weight)*mult > (double)a2->weight)
                        {
                            ok = 1;
                            break;
                        }
                    }

                    if(ok == 2) break;
                    else if(ok == 1) continue;


                    parent->initialMarking[p2] = 0;

                    if(reconstructTrace)
                    {
                        for(auto t : place2.consumers)
                        {
                            std::string tname = getTransitionName(t);
                            const ArcIter arc = getInArc(p2, getTransition(t));
                            _extraconsume[tname].emplace_back(getPlaceName(p2), arc->weight);
                        }
                    }

                    continueReductions = true;
                    _ruleC++;
                    // UC1. Remove p2
                    skipPlace(p2);
                    break;
                }
            }
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleD(uint32_t* placeInQuery) {
        // Rule D - two transitions with the same pre and post and same inhibitor arcs 
        // This does not alter the trace.
        bool continueReductions = false;
        const size_t ntrans = parent->numberOfTransitions();
        for (uint32_t touter = 0; touter < ntrans; ++touter) {
            if(hasTimedout()) return false;
            Transition& tout = getTransition(touter);
            if (tout.skip) continue;

            // D2. No inhibitors
            if (tout.inhib) continue;

            for (uint32_t tinner = touter + 1; tinner < ntrans; ++tinner) {

                Transition& tin = getTransition(tinner);
                if (tin.skip || tout.skip) continue;

                // D2. No inhibitors
                if (tin.inhib) continue;

                for (size_t swp = 0; swp < 2; ++swp) {

                    if (tin.skip || tout.skip) break;
                    
                    uint t1 = touter;
                    uint t2 = tinner;
                    if (swp == 1) std::swap(t1, t2);

                    // D1. not same transition
                    assert(t1 != t2);

                    Transition& trans1 = getTransition(t1);
                    Transition& trans2 = getTransition(t2);

                    // From D3, and D4 we have that pre and post-sets are the same
                    if (trans1.post.size() != trans2.post.size()) break;
                    if (trans1.pre.size() != trans2.pre.size()) break;

                    int ok = 0;
                    uint mult = std::numeric_limits<uint>::max();
                    // D4. postsets must match
                    for (int i = trans1.post.size() - 1; i >= 0; --i) {
                        Arc& arc = trans1.post[i];
                        Arc& arc2 = trans2.post[i];
                        if (arc2.place != arc.place) {
                            ok = 2;
                            break;
                        }

                        if (mult == std::numeric_limits<uint>::max()) {
                            if (arc2.weight < arc.weight || (arc2.weight % arc.weight) != 0) {
                                ok = 1;
                                break;
                            } else {
                                mult = arc2.weight / arc.weight;
                            }
                        } else if (arc2.weight != arc.weight * mult) {
                            ok = 2;
                            break;
                        }
                    }

                    if (ok == 2) break;
                    else if (ok == 1) continue;

                    // D3. Presets must match
                    for (int i = trans1.pre.size() - 1; i >= 0; --i) {
                        Arc& arc = trans1.pre[i];
                        Arc& arc2 = trans2.pre[i];
                        if (arc2.place != arc.place) {
                            ok = 2;
                            break;
                        }

                        if (mult == std::numeric_limits<uint>::max()) {
                            if (arc2.weight < arc.weight || (arc2.weight % arc.weight) != 0) {
                                ok = 1;
                                break;
                            } else {
                                mult = arc2.weight / arc.weight;
                            }
                        } else if (arc2.weight != arc.weight * mult) {
                            ok = 2;
                            break;
                        }
                    }

                    if (ok == 2) break;
                    else if (ok == 1) continue;

                    // UD1. Remove transition t2
                    continueReductions = true;
                    _ruleD++;
                    skipTransition(t2);
                    break; // break the swap loop
                }
            }
        } // end of main for loop for rule D
        assert(consistent());
        return continueReductions;
    }
    
    bool Reducer::ReducebyRuleE(uint32_t* placeInQuery) {
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();
        for(uint32_t p = 0; p < numberofplaces; ++p)
        {
            if(hasTimedout()) return false;
            Place& place = parent->_places[p];
            if(place.skip) continue;
            if(place.inhib) continue;
            if(placeInQuery[p] > 0) continue;
            if(place.producers.size() > place.consumers.size()) continue;
            
            std::set<uint32_t> notenabled;
            bool ok = true;            
            for(uint cons : place.consumers)
            {
                Transition& t = getTransition(cons);
                if(getInArc(p, t)->weight <= parent->initialMarking[p])
                {
                    ok = false;
                    break;
                }               
                else
                {
                    notenabled.insert(cons);
                }
            }
            
            if(!ok) continue;

            for(uint prod : place.producers)
            {
                if(notenabled.count(prod) == 0)
                {
                    ok = false;
                    break;
                }
                // check that producing arcs originate from transition also 
                // consuming. If so, we know it will never fire.
                Transition& t = getTransition(prod);
                ArcIter it = getInArc(p, t);
                if(it == t.pre.end())
                {
                    ok = false;
                    break;
                }
            }
            
            if(!ok) continue;

            _ruleE++;
            continueReductions = true;
            
            parent->initialMarking[p] = 0;
            
            bool skipplace = notenabled.size() == place.consumers.size();
            for(uint cons : notenabled)
                skipTransition(cons);

            if(skipplace)
                skipPlace(p);
            
        }
        assert(consistent());
        return continueReductions;
    }
   
    bool Reducer::ReducebyRuleF(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers) {
        bool reduced = false;
        if(remove_loops)
        {
            std::vector<uint32_t> wtrans;
            std::vector<bool> tseen(parent->numberOfTransitions(), false);

            for(uint32_t p = 0; p < parent->numberOfPlaces(); ++p)
            {
                if(hasTimedout()) return false;
                if(placeInQuery[p] > 0)
                {
                    const Place& place = parent->_places[p];
                    for(auto t : place.consumers)
                    {
                        if(!tseen[t])
                        {
                            wtrans.push_back(t);
                            tseen[t] = true;
                        }
                    }
                    for(auto t : place.producers)
                    {
                        if(!tseen[t])
                        {
                            wtrans.push_back(t);
                            tseen[t] = true;
                        }
                    }
                }
            }

            std::vector<bool> pseen(parent->numberOfPlaces(), false);        
            while(!wtrans.empty())
            {
                if(hasTimedout()) return false;
                auto t = wtrans.back();
                wtrans.pop_back();
                const Transition& trans = parent->_transitions[t];
                for(const Arc& arc : trans.pre)
                {
                    const Place& place = parent->_places[arc.place];
                    if(arc.inhib)
                    {
                        for(auto pt : place.consumers)
                        {
                            if(!tseen[pt])
                            {
                                tseen[pt] = true;
                                wtrans.push_back(pt);                                    
                            }
                        }                        
                    }
                    else
                    {
                        if(!pseen[arc.place])
                        {
                            for(auto pt : place.producers)
                            {
                                if(!tseen[pt])
                                {                                    
                                    Transition& trans = parent->_transitions[pt];
                                    auto it = trans.pre.begin();
                                    for(; it != trans.pre.end(); ++it)
                                        if(it->place >= arc.place) break;
                                    
                                    if(it != trans.pre.end() && it->place == arc.place && !it->inhib)
                                    {
                                        auto it2 = trans.post.begin();
                                        for(; it2 != trans.post.end(); ++it2)
                                            if(it2->place >= arc.place) break;
                                        if(it->weight >= it2->weight) continue;
                                    }

                                    tseen[pt] = true;
                                    wtrans.push_back(pt);
                                }
                            }

                            for(auto pt : place.consumers)
                            {
                                if(!tseen[pt] && (!remove_consumers || placeInQuery[pt] > 0))
                                {
                                    tseen[pt] = true;
                                    wtrans.push_back(pt);                                    
                                }
                            }
                        }
                        pseen[arc.place] = true;
                    }
                }
            }

            for(size_t t = 0; t < parent->numberOfTransitions(); ++t)
            {
                if(!tseen[t] && !parent->_transitions[t].skip)
                {
                    ++_ruleF;
                    skipTransition(t);
                    reduced = true;
                }
            }

            for(size_t p = 0; p < parent->numberOfPlaces(); ++p)
            {
                if(!pseen[p] && !parent->_places[p].skip && placeInQuery[p] == 0)
                {
                    assert(placeInQuery[p] == 0);
                    ++_ruleF;
                    skipPlace(p);
                    reduced = true;
                }
            }
        }
        else
        {            
            const size_t numberofplaces = parent->numberOfPlaces();
            for(uint32_t p = 0; p < numberofplaces; ++p)
            {
                if(hasTimedout()) return false;
                Place& place = parent->_places[p];
                if(place.skip) continue;
                if(place.inhib) continue;
                if(placeInQuery[p] > 0) continue;
                if(place.consumers.size() > 0) continue;

                ++_ruleF;
                reduced = true;
                skipPlace(p);
                std::vector<uint32_t> torem;
                for(auto& t : place.producers)
                {
                    auto& trans = parent->_transitions[t];
                    if(trans.post.size() != 0)
                        continue;
                    bool ok = true;
                    for(auto& a : trans.pre)
                    {
                        if(placeInQuery[a.place] > 0)
                        {
                            ok = false;
                        }
                    }
                    if(ok) torem.push_back(t);
                }

                for(auto t : torem)
                    skipTransition(t);
                assert(consistent());
            }
        }
        
        return reduced;
    }
   
    bool Reducer::ReducebyRuleG(uint32_t* placeInQuery) {
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();
        for(uint32_t p = 0; p < numberofplaces; ++p)
        {
            if(hasTimedout()) return false;
            Place& place = parent->_places[p];
            if(place.skip) continue;
            if(place.inhib) continue;
            if(place.producers.size() < place.consumers.size()) continue;
            if(placeInQuery[p] != 0) continue; 
            
            bool ok = true;
            for(uint cons : place.consumers)
            {
                Transition& t = getTransition(cons);
                auto w = getInArc(p, t)->weight;
                if(w > parent->initialMarking[p])
                {
                    ok = false;
                    break;
                }               
                else
                {
                    auto it = getOutArc(t, p);
                    if(it == t.post.end() || 
                       it->place != p     ||
                       it->weight < w)
                    {
                        ok = false;
                        break;
                    }
                }
            }
            
            if(!ok) continue;
            
            ++_ruleG;
            
            if((numberofplaces - _removedPlaces) > 1)
            {
                skipPlace(p);
                continueReductions = true;
            }
            
        }
        assert(consistent());
        return continueReductions;
    }
    
    
    bool Reducer::ReducebyRuleH(uint32_t* placeInQuery, bool remove_loops) {
        if(!remove_loops) return false;
        bool continueReductions = false;
        for(uint32_t t = 0; t < parent->numberOfTransitions(); ++t)
        {
            if(hasTimedout()) return false;
            Transition& trans = parent->_transitions[t];
            if(trans.skip) continue;
            if(trans.inhib) continue;
            if(trans.pre.size() < trans.post.size()) continue;
            if(!remove_loops && trans.pre.size() == 0) continue;
            
            auto postit = trans.post.begin();
            auto preit = trans.pre.begin();
            
            bool ok = true;
            while(true)
            {
                if(preit == trans.pre.end() && postit == trans.post.end())
                    break;
                if(preit == trans.pre.end())
                {
                    ok = false;
                    break;
                }
                if(preit->inhib)
                {
                    ok = false;
                    break;
                }
                if(postit != trans.post.end() && preit->place == postit->place)
                {
                    if((placeInQuery[preit->place] > 0 && preit->weight != postit->weight) ||
                       (placeInQuery[preit->place] == 0 && preit->weight < postit->weight))
                    {
                        ok = false;
                        break;
                    }
                    ++preit;
                    ++postit;
                }
                else if(postit == trans.post.end() || preit->place < postit->place) 
                {
                    if(placeInQuery[preit->place] > 0)
                    {
                        ok = false;
                        break;
                    }
                    ++preit;
                }
                else
                {
                    // could not match a post with a pre
                    ok = false;
                    break;
                }
            }
                        
            if(!ok) continue;
            ++_ruleH;
            skipTransition(t);
        }
        assert(consistent());
        return continueReductions;
    }
    
    bool Reducer::ReducebyRuleI(uint32_t* placeInQuery)
    {
        bool continueReductions = false;
        for(uint32_t t1 = 0; t1 < parent->numberOfTransitions(); ++t1)
        {
            auto& trans1 = parent->_transitions[t1];
            if(trans1.skip) continue;
            if(trans1.inhib) continue;
            if(trans1.pre.size() != 1) continue;
            if(trans1.post.size() != 1) continue;
            auto p1 = trans1.pre[0].place;
            auto p2 = trans1.post[0].place;
            if(trans1.pre[0].weight != 1) continue;
            if(trans1.post[0].weight != 1) continue;
            if(p1 == p2) continue;
            if(placeInQuery[p1] > 0) continue;
            if(placeInQuery[p2] > 0) continue;
            if(parent->_places[p1].inhib) continue;
            if(parent->_places[p2].inhib) continue;
            
            for(uint32_t t2 = t1 + 1; t2 < parent->numberOfTransitions(); ++t2)
            {
                auto& trans2 = parent->_transitions[t2];
                if(trans2.skip) continue;
                if(trans2.inhib) continue;
                if(trans2.pre.size() != 1) continue;
                if(trans2.post.size() != 1) continue;
                if(trans2.pre[0].weight != 1) continue;
                if(trans2.post[0].weight != 1) continue;
                if(trans2.pre[0].place != p2) continue;                
                if(trans2.post[0].place != p1) continue;                

               ++_ruleI;
                skipTransition(t2);

                auto& place1 = parent->_places[p1];
                auto& place2 = parent->_places[p2];

                {

                    for(auto p2it : place2.consumers)
                    {
                        auto& t = parent->_transitions[p2it];
                        auto arc = getInArc(p2, t);
                        assert(arc != t.pre.end());
                        assert(arc->place == p2);
                        auto a = *arc;
                        a.place = p1;
                        auto dest = std::lower_bound(t.pre.begin(), t.pre.end(), a);
                        if(dest == t.pre.end() || dest->place != p1)
                        {
                            t.pre.insert(dest, a);
                            auto lb = std::lower_bound(place1.consumers.begin(), place1.consumers.end(), p2it);
                            place1.consumers.insert(lb, p2it);
                        }
                        else
                        {
                            dest->weight += a.weight;
                        }
                        consistent();
                    }
                }
                
                {
                    auto p2it = place2.producers.begin();

                    for(;p2it != place2.producers.end(); ++p2it)
                    {
                        auto& t = parent->_transitions[*p2it];
                        Arc a = *getOutArc(t, p2);
                        a.place = p1;
                        auto dest = std::lower_bound(t.post.begin(), t.post.end(), a);
                        if(dest == t.post.end() || dest->place != p1)
                        {
                            t.post.insert(dest, a);
                            auto lb = std::lower_bound(place1.producers.begin(), place1.producers.end(), *p2it);
                            place1.producers.insert(lb, *p2it);
                        }
                        else
                        {
                            dest->weight += a.weight;
                        }
                        consistent();
                    }
                }
                
                parent->initialMarking[p1] += parent->initialMarking[p2];
                
                skipPlace(p2);
                continueReductions = true;
            }
        }   
        return continueReductions;
    }
    
    void Reducer::Reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace, int timeout, bool remove_loops, bool remove_consumers) {
        this->_timeout = timeout;
        _timer = std::chrono::high_resolution_clock::now();
        assert(consistent());
        this->reconstructTrace = reconstructTrace;
        if (enablereduction == 1) { // in the aggressive reduction all four rules are used as long as they remove something
            bool changed = false;
            do
            {
                if(remove_loops)
                    ReducebyRuleF(context.getQueryPlaceCount(), remove_loops, remove_consumers);
                do{
                    changed = false;
                    changed |= ReducebyRuleA(context.getQueryPlaceCount());
                    changed |= ReducebyRuleB(context.getQueryPlaceCount());
                    changed |= ReducebyRuleE(context.getQueryPlaceCount());
                    changed |= ReducebyRuleG(context.getQueryPlaceCount());
                    changed |= ReducebyRuleH(context.getQueryPlaceCount(), remove_loops);
                    if(!remove_loops) 
                        changed |= ReducebyRuleF(context.getQueryPlaceCount(), remove_loops, remove_consumers);
                } while(changed && !hasTimedout());
                // RuleC and RuleD are expensive, so wait with those till nothing else changes
                changed |= ReducebyRuleD(context.getQueryPlaceCount());
                changed |= ReducebyRuleC(context.getQueryPlaceCount());
                if(!changed)
                    // Only try RuleI last. It can reduce applicability of other rules.
                    changed |= ReducebyRuleI(context.getQueryPlaceCount());
            } while(!hasTimedout() && changed);

        } else if (enablereduction == 2) { // for k-boundedness checking only rules A and D are applicable
            while ( (
                        ReducebyRuleA(context.getQueryPlaceCount()) ||
                        ReducebyRuleD(context.getQueryPlaceCount())
                    ) && !hasTimedout()) {
            }
        }
    }
    
    void Reducer::postFire(std::ostream& out, const std::string& transition)
    {
        if(_postfire.count(transition) > 0)
        {
            std::queue<std::string> tofire;
            
            for(auto& el : _postfire[transition]) tofire.push(el);
            
            for(auto& el : _postfire[transition])
            {
                tofire.pop();
                out << "\t<transition id=\"" << el << "\">\n";
                extraConsume(out, el);
                out << "\t</transition>\n";               
                postFire(out, el);
            }
        }
    }
    
    void Reducer::initFire(std::ostream& out)
    {
        for(std::string& init : _initfire)
        {
            out << "\t<transition id=\"" << init << "\">\n";
            extraConsume(out, init);            
            out << "\t</transition>\n";
            postFire(out, init);
        }
    }
    
    void Reducer::extraConsume(std::ostream& out, const std::string& transition)
    {
        if(_extraconsume.count(transition) > 0)
        {
            for(auto& ec : _extraconsume[transition])
            {
                out << ec;
            }
        }
    }

} //PetriNet namespace
