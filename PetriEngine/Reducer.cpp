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
            
            if(place.skip) continue;    // allready removed    
            
            // B4 and B6. dont mess up query and initial state
            if(placeInQuery[p] > 0 || parent->initMarking()[p] > 0) continue; // to important                   
            
            // B2. Only one consumer/producer
            if(place.consumers.size() != 1 || place.producers.size() != 1) continue; // no orphan removal
            
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
            
            // B5. Do inhibitor check, neither In, out or place can be involved with any inhibitor
            if(place.inhib || in.inhib || out.inhib) continue;
            
            // B7. also, none of the places in the post-set of consuming transition can be participating in inhibitors.
            // B8. nor can they appear in the query.
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
            }
            
            continueReductions = true;
            _ruleB++;
             // UB1. Remove place p
            skipPlace(p);
            _removedPlaces++;

            // We need to remember that when tOut fires, tIn fires just after.
            // this should fix the trace
            
            // UB2. move arcs from t' to t
            for (auto& arc : in.post) { // remove tPost
                auto _arc = getOutArc(out, arc.place);
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
            // Already removed
            if(place1.skip) continue;
                        
            // C2. Only one intoing arc
            if(place1.producers.size() != 1) continue;
            uint tin = place1.producers[0];

            uint tout = std::numeric_limits<uint>::max();
            for(uint cons : place1.consumers)
            {
                auto t = getTransition(cons);
                auto a = getInArc(p1, t);
                
                if(!a->inhib && !a->skip)
                {
                    if(tout != std::numeric_limits<uint>::max()) 
                    {
                        tout = std::numeric_limits<uint>::max();
                        break;
                    }
                    tout = cons;
                }
            }
            
            // C1, C3. Consumer, producer must differ, there must be 
            // one (non-inhibiting) outgoing arc
            if( tin == tout || 
                tout == std::numeric_limits<uint>::max())
            {
                continue;
            }
            
            // we could probably be clever and use symmetry here
            for (uint32_t p2 = 0; p2 < parent->numberOfPlaces(); ++p2) 
            {
                // C1. places have to differ
                if(p1 == p2) continue;
                // C6. agree on initial marking
                if(parent->initMarking()[p1] != parent->initMarking()[p2]) 
                {
                    continue;
                }

                // C8. place we remove cannot be in query
                if(placeInQuery[p2] > 0)
                {
                    continue;
                }
 
                Place& place2 = parent->_places[p2];
                
                // already removed
                if(place2.skip)
                {
                    continue;
                }
                
                // C7. either they agree on inhibiting, or removed place is non-inhibit
                if(place2.inhib != place1.inhib && place2.inhib)
                {
                    continue;
                }
                
                // C2, C3. Same number of ingoing/outgoing (including inhib)
                if(place1.consumers.size() != place2.consumers.size() ||
                   place1.producers.size() != place2.producers.size())
                {
                    continue;
                }

                // C3. Only one input
                if(place2.producers.size() != 1) continue;
                uint tin2 = place2.producers[0];

                // C2. inputs must match
                if(tin2 != tin)
                {
                    continue;
                }
                
                uint tout2 = std::numeric_limits<uint>::max();
                for(uint cons : place2.consumers)
                {
                    auto t = getTransition(cons);
                    auto a = getInArc(p2, t);
                    if(!a->inhib && !a->skip) 
                    {
                        if(tout2 != std::numeric_limits<uint>::max()) 
                        {
                            tout2 = std::numeric_limits<uint>::max();
                            break;
                        }
                        tout2 = cons;
                    }
                    
                    if(place2.inhib)
                    {
                        // C7. We must make sure inhibitors match
                        if(std::find(place1.consumers.begin(), place1.consumers.end(), cons) == place1.consumers.end())
                        {
                            tout2 = std::numeric_limits<uint>::max();
                            break;
                        }
                        // C7. also weights of inhib must match
                        if(getInArc(p1, t)->weight != getInArc(p2, t)->weight)
                        {
                            tout2 = std::numeric_limits<uint>::max();
                            break;
                        }
                    }
                }

                // C3. Outputs must match
                if(tout2 != tout)
                {
                    continue;
                }
                                
                // C4, C5. Match between weights
                Transition& t1 = getTransition(place1.consumers[0]);
                Transition& t2 = getTransition(place1.producers[0]);
                if(getInArc(p1, t1)->weight != getOutArc(t2, p1)->weight ||
                   getInArc(p2, t1)->weight != getOutArc(t2, p2)->weight)
                {
                    continue;
                }          
                                            
                // We need to remember to consume from p2 when place1.consumers[0] fires
                // otherwise, trace is inconsistent
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
                // UC1. Remove place
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
            
            // already skipped
            if(trans1.skip) continue;

            // use symmetry
            for (uint32_t t2 = t1+1; t2 < parent->numberOfTransitions(); ++t2) {
                
                // D1. Need to differ
                if(t1 == t2) continue;
                
                Transition& trans2 = getTransition(t2);
                
                // already removed
                if(trans2.skip) continue;
                
                // D2. Same number of inputs
                if(trans1.pre.size() != trans2.pre.size()) continue;
                
                // D3. Same number of outputs
                if(trans1.post.size() != trans2.post.size()) continue;

                bool ok = true;
                // D2. same weights on inputs, and
                // D4. agree on inhibiting
                for(auto& arc : trans1.pre)
                {
                    auto a2 = getInArc(arc.place, trans2);
                    if( a2 == trans2.pre.end() ||
                        a2->weight != arc.weight ||
                        a2->inhib != arc.inhib)
                    {
                        ok = false;
                    }
                }
                if (!ok) continue;
                // D3. same weights on inputs                
                for(auto& arc : trans1.post)
                {
                    auto a2 = getOutArc(trans2, arc.place);
                    if( a2 == trans2.post.end() ||
                        a2->weight != arc.weight)
                    {
                        ok = false;
                    }
                }
                                
                if (!ok) continue;
                
                // Remove transition t2
                continueReductions = true;
                _ruleD++;
                _removedTransitions++;
                skipTransition(t2);
            }
        } // end of main for loop for rule D
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
