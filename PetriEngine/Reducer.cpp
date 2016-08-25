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

namespace PetriEngine {

    Reducer::Reducer(PetriNetBuilder* p) 
    : _removedTransitions(0), _removedPlaces(0), _ruleA(0), _ruleB(0), _ruleC(0), _ruleD(0), parent(p) {
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
            for(auto& arc : parent->_transitions[t].pre)
            {
                if (arc.weight > 0) 
                    std::cout   << "   Input place " << arc.place 
                                << " with arc-weight " << arc.weight << std::endl;
            }
            for(auto& arc : parent->_transitions[t].post)
            {
                if (arc.weight > 0) 
                    std::cout   << "  Output place " << arc.place 
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
    Transition& Reducer::getTransition(uint32_t transition)
    {
        return parent->_transitions[transition];
    }
    
    ArcIter Reducer::getOutArc(Transition& trans, uint32_t place)
    {
        auto it = trans.post.begin();
        for(; it != trans.post.end(); ++it)
        {
            if(it->place == place)
            {
                return it;
            }
        }
        return it;
    }
    
    ArcIter Reducer::getInArc(uint32_t place, Transition& trans)
    {
        auto it = trans.pre.begin();
        for(; it != trans.pre.end(); ++it)
        {
            if(it->place == place)
            {
                return it;
            }
        }
        return it;
    }
    
    void Reducer::eraseTransition(std::vector<uint32_t>& set, uint32_t el)
    {
        for(auto it = set.begin(); it != set.end(); ++it )
        {
            if(*it == el)
            {
                set.erase(it);
                return;
            }
        }
    }
    void Reducer::skipTransition(uint32_t t)
    {
        Transition& trans = getTransition(t);
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
    }
    
    void Reducer::skipPlace(uint32_t place)
    {
        Place& pl = parent->_places[place];
        pl.skip = true;
        for(auto& t : pl.consumers)
        {
            Transition& trans = getTransition(t);
            for(auto it = trans.pre.begin(); it != trans.pre.end(); ++it)
            {
                if(it->place == place)
                {
                    trans.pre.erase(it);
                    break;
                }
            }
        }
        
        for(auto& t : pl.producers)
        {
            Transition& trans = getTransition(t);
            for(auto it = trans.post.begin(); it != trans.post.end(); ++it)
            {
                if(it->place == place)
                {
                    trans.post.erase(it);
                    break;
                }
            }
        }
    }
    

    bool Reducer::ReducebyRuleA(uint32_t* placeInQuery) {
        // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places (and t)  
        bool continueReductions = false;
        for (uint32_t t = 0; t < parent->numberOfTransitions(); t++) {

            Transition& trans = getTransition(t);
            
            // should skip, or we have more than one in pre or post
            if(trans.skip || trans.pre.size() != 1 || trans.post.size() != 1) continue;
            
            // We have weight of more than one
            if(trans.pre[0].weight != 1 || trans.post[0].weight != 1) continue;
            
            uint32_t pPre = trans.pre[0].place;
            uint32_t pPost = trans.post[0].place;
            
            // dont mess with query!
            if(placeInQuery[pPre] > 0 || placeInQuery[pPost]) continue;
            
            // cant remove if both are in initmarking
            if(parent->initMarking()[pPre] > 0 && parent->initMarking()[pPost] > 0) continue;
                 
            // continue if we didn't find unique pPre and pPost that are different
            if (pPre < 0 || pPost < 0 || pPre == pPost) continue;

            // Check that pPre goes only to t
            if(parent->_places[pPre].consumers.size() != 1) continue;
            
            // Do inhibitor check, neither T, pPre or pPost can be involved with any inhibitor
            if(parent->_places[pPre].inhib || parent->_places[pPost].inhib || trans.inhib) continue;
            

            continueReductions = true;
            _ruleA++;

            // Remove transition t and the place that has no tokens in m0
            _removedTransitions++; 
            skipTransition(t);
            assert(pPost != pPre);
            if (parent->initMarking()[pPre] == 0) { // removing pPre
                for(auto& _t : parent->_places[pPre].producers)
                {
                    assert(_t != t);
                    // move output-arcs to post.
                    Transition& trans = getTransition(_t);
                    auto source = getOutArc(trans, pPre);
                    auto dest = getOutArc(trans, pPost);
                    if(dest == trans.post.end())
                    {
                        source->place = pPost;
                        parent->_places[pPost].producers.push_back(_t);
                    }
                    else
                    {
                        dest->weight += source->weight;

                    }
                }                
                _removedPlaces++;
                skipPlace(pPre);
            } else if (parent->initMarking()[pPost] == 0) { // removing pPost
                parent->_places[pPost].skip = true;
                for (auto& _t : parent->_places[pPost].consumers) {
                    assert(_t != t);
                    Transition& trans = getTransition(_t);
                    auto source = getInArc(pPost, trans);
                    auto dest = getInArc(pPre, trans);
                    if(dest == trans.pre.end())
                    {
                        source->place = pPre;
                        parent->_places[pPre].consumers.push_back(_t);
                    }
                    else
                    {
                        dest->weight += source->weight;
                    }
                }
                
                for(auto& _t : parent->_places[pPost].producers)
                {
                    if(_t == t) continue;
                    Transition& trans = getTransition(_t);
                    auto source = getOutArc(trans, pPost);
                    auto dest = getOutArc(trans, pPre);
                    
                    if(dest == trans.post.end())
                    {
                        source->place = pPre;
                        parent->_places[pPre].producers.push_back(_t);
                    }
                    else
                    {
                        dest->weight += source->weight;
                    }
                }

                skipPlace(pPost);
                _removedPlaces++;
            }
        } // end of Rule A main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleB(uint32_t* placeInQuery) {
        // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
        bool continueReductions = false;
        for (uint32_t p = 0; p < parent->numberOfPlaces(); p++) {
            
            Place& place = parent->_places[p];
            
            if(place.skip) continue;    // allready removed           
            if(placeInQuery[p] > 0 || parent->initMarking()[p] > 0) continue; // to important                   
            if(place.consumers.size() != 1 || place.producers.size() != 1) continue; // no orphan removal
            
            int tOut = place.producers[0];
            int tIn = place.consumers[0];
            
            if (tOut == tIn) continue; // cannot remove this kind either
                        
            Transition& out = getTransition(tOut);
            Transition& in = getTransition(tIn);
            
            auto inArc = getInArc(p, in);
            auto outArc = getOutArc(out, p);
            
            if(inArc->weight != outArc->weight) continue;
            
            // Do inhibitor check, neither In, out or place can be involved with any inhibitor
            if(place.inhib || in.inhib || out.inhib) continue;
            
            // also, none of the places in the post-set of out can be participating in inhibitors.
            {
                bool post_inhib = false;
                for(const Arc& a : out.post)
                {
                    post_inhib |= parent->_places[a.place].inhib;
                    if(post_inhib) break;
                }
                if(post_inhib) continue;
            }

            bool ok = true;
            // Check if the output places of tPost do not have any inhibitor arcs connected and are not used in the query
            for (auto& arc : in.post) {
                if (arc.weight > 0 &&  placeInQuery[arc.place] > 0) {
                    ok = false;
                    break;
                }
            }
            // Check that there is no other place than p that gives to tPost, tPre can give to other places
            for (auto& arc : in.pre) {
                if (arc.weight > 0 && arc.place != p) {
                    ok = false;
                    break;
                }
            }
            if (!ok) {
                continue;
            }
            continueReductions = true;
            _ruleB++;
             // Remove place p
            skipPlace(p);
            _removedPlaces++;

            for (auto& arc : in.post) { // remove tPost
                auto _arc = getOutArc(out, arc.place);
                if(_arc != out.post.end())
                {
                    _arc->weight += arc.weight;
                }
                else
                {
                    out.post.push_back(arc);
                    parent->_places[arc.place].producers.push_back(tOut);
                }
            }
            _removedTransitions++;
            skipTransition(tIn);
        } // end of Rule B main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleC(uint32_t* placeInQuery) {
        // Rule C - Places with same input and output-transitions which a modulo each other
         bool continueReductions = false;
        
        for(uint32_t p1 = 0; p1 < parent->numberOfPlaces(); ++p1)
        {
            if( placeInQuery[p1] > 0 || parent->initMarking()[p1])
                continue;
            
            Place& place1 = parent->_places[p1];
            
            if(place1.skip || place1.inhib) continue;
            
            // use symmetry to speed up things
            for (uint32_t p2 = p1 + 1; p2 < parent->numberOfPlaces(); ++p2) 
            {
                if(p1 == p2) continue;
                
                if(placeInQuery[p2] > 0 || parent->initMarking()[p2])
                    continue;
                
                Place& place2 = parent->_places[p2];
                
                if(place2.skip || place2.inhib) continue;
                
                if(place1.consumers.size() != place2.consumers.size() ||
                   place1.producers.size() != place2.producers.size())
                    continue;
                
                bool ok = true;
                for(auto& in : place1.consumers)
                {
                    Transition& trans = getTransition(in);
                    auto a1 = getInArc(p1, trans);
                    auto a2 = getInArc(p2, trans);
                    if( a2 == trans.pre.end() ||
                        a2->weight != a1->weight)
                    {
                        ok = false;
                    }
                }
                
                if(!ok) continue;
                
                for(auto& in : place1.producers)
                {
                    Transition& trans = getTransition(in);
                    auto a1 = getOutArc(trans, p1);
                    auto a2 = getOutArc(trans, p2);
                    if( a2 == trans.post.end() ||
                        a2->weight != a1->weight)
                    {
                        ok = false;
                    }
                }
                
                if(!ok) continue;
                                
                continueReductions = true;
                _ruleC++;
                skipPlace(p2);
                _removedPlaces++;
            }
        }
        return continueReductions;
    }

    bool Reducer::ReducebyRuleD(uint32_t* placeInQuery) {
        // Rule D - two transitions with the same pre and post and same inhibitor arcs 
        bool continueReductions = false;
        for (uint32_t t1 = 0; t1 < parent->numberOfTransitions(); t1++) {
            Transition& trans1 = getTransition(t1);
            if(trans1.skip) continue;

            for (uint32_t t2 = 0; t2 < parent->numberOfTransitions(); t2++) {
                if(t1 == t2) continue;
                
                Transition& trans2 = getTransition(t2);
                if(trans2.skip) continue;
                
                if(trans1.post.size() != trans2.post.size()) continue;
                if(trans1.pre.size() != trans2.pre.size()) continue;
                
                bool ok = true;
                for(auto& arc : trans1.post)
                {
                    auto a2 = getOutArc(trans2, arc.place);
                    if( a2 == trans2.post.end() ||
                        a2->weight != arc.weight)
                    {
                        ok = false;
                    }
                }
                
                for(auto& arc : trans1.pre)
                {
                    auto a2 = getInArc(arc.place, trans2);
                    if( a2 == trans2.pre.end() ||
                        a2->weight != arc.weight)
                    {
                        ok = false;
                    }
                }
                
                if (!ok) {
                    continue;
                }
                // Remove transition t2
                continueReductions = true;
                _ruleD++;
                _removedTransitions++;
                skipTransition(t2);
            }
        } // end of main for loop for rule D
        return continueReductions;
    }

    void Reducer::Reduce(QueryPlaceAnalysisContext& context, int enablereduction) {
        if (enablereduction == 1) { // in the aggresive reduction all four rules are used as long as they remove something
            while (ReducebyRuleA(context.getQueryPlaceCount()) ||
                    ReducebyRuleB(context.getQueryPlaceCount()) ||
                    ReducebyRuleC(context.getQueryPlaceCount()) ||
                    ReducebyRuleD(context.getQueryPlaceCount())) {
            }
        } else if (enablereduction == 2) { // for k-boundedness checking only rules A and D are applicable
            while (ReducebyRuleA(context.getQueryPlaceCount()) ||
                    ReducebyRuleD(context.getQueryPlaceCount())) {
            }
        }
    }

} //PetriNet namespace
