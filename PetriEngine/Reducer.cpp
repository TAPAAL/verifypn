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

namespace PetriEngine {

    Reducer::Reducer(PetriNetBuilder* p) 
    : _removedTransitions(0), _removedPlaces(0), _ruleA(0), _ruleB(0), _ruleC(0), _ruleD(0), _ruleE(0), parent(p) {
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
                        
            // we have already removed
            if(trans.skip) continue;

            // A2. we have more/less than one arc in pre or post
            // checked first to avoid out-of-bounds when looking up indexes.
            if(trans.pre.size() != 1 || trans.post.size() != 1) continue;

            uint32_t pPre = trans.pre[0].place;
            uint32_t pPost = trans.post[0].place;
            
            // A1. continue if source and destination are the same
            if (pPre == pPost) continue;
            
            // A2. Check that pPre goes only to t
            if(parent->_places[pPre].consumers.size() != 1) continue;
            
            // A3. We have weight of more than one on input
            // and is empty on output (should not happen).
            if(trans.pre[0].weight != 1 || trans.post[0].weight < 1) continue;
                        
            // A4. Do inhibitor check, neither T, pPre or pPost can be involved with any inhibitor
            if(parent->_places[pPre].inhib || parent->_places[pPost].inhib || trans.inhib) continue;

            // A5. dont mess with query!
            if(placeInQuery[pPre] > 0 || placeInQuery[pPost]) continue;
             
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
            
            size_t weight = trans.post[0].weight;
            
            // UA2. move the token for the initial marking, makes things simpler.
            parent->initialMarking[pPost] += (parent->initialMarking[pPre] * weight);
            parent->initialMarking[pPre] = 0;

            // Remove transition t and the place that has no tokens in m0
            _removedTransitions++; 
            // UA1. remove transition
            skipTransition(t);
            assert(pPost != pPre);

            // UA2. update arcs
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
                    source->weight *= weight;
                    parent->_places[pPost].producers.push_back(_t);
                }
                else
                {
                    dest->weight += (source->weight * weight);

                }
            }                
            _removedPlaces++;
            // UA1. remove place
            skipPlace(pPre);
        } // end of Rule A main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleB(uint32_t* placeInQuery) {
        // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
        bool continueReductions = false;
        for (uint32_t p = 0; p < parent->numberOfPlaces(); p++) {
            
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
            _removedPlaces++;
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
                }
            }
            _removedTransitions++;
            // UB1. remove transition
            skipTransition(tIn);
        } // end of Rule B main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleC(uint32_t* placeInQuery) {
        // Rule C - Places with same input and output-transitions which a modulo each other
         bool continueReductions = false;
        
        for(uint32_t p1 = 0; p1 < parent->numberOfPlaces(); ++p1)
        {            
            Place& place1 = parent->_places[p1];
            
            if(place1.skip) continue;
            
            // C4. No inhib
            if(place1.inhib) continue;
            
            for (uint32_t p2 = 0; p2 < parent->numberOfPlaces(); ++p2) 
            {
                // C1. Not same place
                if(p1 == p2) continue;
                
                // C5. Dont mess with query
                if(placeInQuery[p2] > 0)
                    continue;
                
                Place& place2 = parent->_places[p2];
                
                if(place2.skip) continue;

                // C4. No inhib
                if(place2.inhib) continue;
                
                // C2, C3. Consumer and producer-sets must match
                if(place1.consumers.size() != place2.consumers.size() ||
                   place1.producers.size() != place2.producers.size())
                    continue;
                
                uint mult = std::numeric_limits<uint>::max();
                
                // C8. Consumers must match with weights
                bool ok = true;
                for(auto& in : place1.consumers)
                {
                    Transition& trans = getTransition(in);
                    auto a1 = getInArc(p1, trans);
                    auto a2 = getInArc(p2, trans);
                    if(a2 == trans.pre.end())
                    {
                        ok = false;
                        break;
                    }
                    
                    if(mult == std::numeric_limits<uint>::max())
                    {
                        if(a2->weight < a1->weight || (a2->weight % a1->weight) != 0)
                        {
                            ok = false;
                            break;
                        }
                        mult = a2->weight / a1->weight;
                    }
                    else if(a1->weight != a2->weight)
                    {
                        ok = false;
                        break;
                    }
                }
                
                if(!ok) continue;
                
                // C7. Producers must match with weights
                for(auto& in : place1.producers)
                {
                    Transition& trans = getTransition(in);
                    auto a1 = getOutArc(trans, p1);
                    auto a2 = getOutArc(trans, p2);
                    if( a2 == trans.post.end())
                    {
                        ok = false;
                        break;
                    }
                    if(mult == std::numeric_limits<uint>::max())
                    {
                        if(a2->weight < a1->weight || (a2->weight % a1->weight) != 0)
                        {
                            ok = false;
                            break;
                        }
                        mult = a2->weight / a1->weight;
                    }
                    else if(a1->weight != a2->weight)
                    {
                        ok = false;
                        break;
                    }
                }
                
                // C6. We do not care about excess markings in p2.
                if(mult != std::numeric_limits<uint>::max() &&
                        (parent->initialMarking[p1] * mult) > parent->initialMarking[p2])
                    continue;

                if(!ok) continue;
                
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
                _removedPlaces++;
            }
        }
        return continueReductions;
    }

    bool Reducer::ReducebyRuleD(uint32_t* placeInQuery) {
        // Rule D - two transitions with the same pre and post and same inhibitor arcs 
        // This does not alter the trace.
        bool continueReductions = false;
        for (uint32_t t1 = 0; t1 < parent->numberOfTransitions(); ++t1) {
            Transition& trans1 = getTransition(t1);
            if(trans1.skip) continue;

            // D2. No inhibitors
            if(trans1.inhib) continue;
            
            for (uint32_t t2 = 0; t2 < parent->numberOfTransitions(); ++t2) {
                
                // D1. not same transition
                if(t1 == t2) continue;
                                
                Transition& trans2 = getTransition(t2);
                if(trans2.skip) continue;
                
                // D2. No inhibitors
                if(trans2.inhib) continue;
                
                // From D3, and D4 we have that pre and post-sets are the same
                if(trans1.post.size() != trans2.post.size()) continue;
                if(trans1.pre.size() != trans2.pre.size()) continue;
                
                bool ok = true;
                uint mult = std::numeric_limits<uint>::max();
                // D4. postsets must match
                for(auto& arc : trans1.post)
                {
                    auto a2 = getOutArc(trans2, arc.place);
                    if( a2 == trans2.post.end())
                    {
                        ok = false;
                        break;
                    }
                    
                    if(mult == std::numeric_limits<uint>::max())
                    {
                        if(a2->weight < arc.weight || (a2->weight % arc.weight) != 0)
                        {
                            ok = false;
                            break;
                        }
                        else
                        {
                            mult = a2->weight / arc.weight;
                        }
                    }
                    else if(a2->weight != arc.weight*mult)
                    {
                        ok = false;
                        break;
                    }
                }
                
                // D3. Presets must match
                for(auto& arc : trans1.pre)
                {
                    auto a2 = getInArc(arc.place, trans2);
                    if( a2 == trans2.pre.end())
                    {
                        ok = false;
                        break;
                    }

                    if(mult == std::numeric_limits<uint>::max())
                    {
                        if(a2->weight < arc.weight || (a2->weight % arc.weight) != 0)
                        {
                            ok = false;
                            break;
                        }
                        else
                        {
                            mult = a2->weight / arc.weight;
                        }
                    }
                    else if(a2->weight != arc.weight*mult)
                    {
                        ok = false;
                        break;
                    }
                }
                                
                if (!ok) {
                    continue;
                }
                
                // UD1. Remove transition t2
                continueReductions = true;
                _ruleD++;
                _removedTransitions++;
                skipTransition(t2);
            }
        } // end of main for loop for rule D
        return continueReductions;
    }
    
    bool Reducer::ReducebyRuleE(uint32_t* placeInQuery) {
        bool continueReductions = false;
        for(uint32_t p = 0; p < parent->numberOfPlaces(); ++p)
        {
            Place& place = parent->_places[p];
            
            if(place.skip) continue;
            if(place.inhib) continue;
            if(placeInQuery[p] > 0) continue;
            if(place.producers.size() > place.consumers.size()) continue;
            
            bool ok = true;
            for(uint cons : place.consumers)
            {
                Transition& t = getTransition(cons);
                if(getInArc(p, t)->weight <= parent->initialMarking[p])
                {
                    ok = false;
                    break;
                }               
            }
            
            if(!ok) continue;
            
            for(uint prod : place.producers)
            {
                // check that producing arcs originate from transition also 
                // consuming. If so, we know it will never fire.
                Transition& t = getTransition(prod);
                ArcIter it = getInArc(p, t);
                if(it != t.post.end())
                {
                    ok = false;
                    break;
                }
            }
            
            if(!ok) continue;
            
            parent->initialMarking[p] = 0;
            
            auto torem = place.consumers;
            for(uint cons : torem)
            {
                skipTransition(cons);                
                _removedTransitions++;
            }
            
            skipPlace(p);
            _removedPlaces++;
            _ruleE++;
            continueReductions = true;
        }
        return continueReductions;
    }


    void Reducer::Reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace) {
        this->reconstructTrace = reconstructTrace;
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
    
    void Reducer::postFire(std::ostream& out, std::string transition)
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
    
    void Reducer::extraConsume(std::ostream& out, std::string transition)
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
