/*
 * File:   Reducer.cpp
 * Authors:
 *      Jiri Srba
 *      Jesper Adriaan van Diepen
 *      Nicolaj Østerby Jensen
 *      Mathias Mehl Sørensen
 *
 * Created on 15 February 2014, 10:50
 * Updated 7 March 2022
 */

#include "PetriEngine/Reducer.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriParse/PNMLParser.h"
#include <queue>
#include <set>
#include <algorithm>
#include <numeric>

namespace PetriEngine {

    Reducer::Reducer(PetriNetBuilder* p)
    : parent(p) {
    }

    Reducer::~Reducer() {

    }

    void Reducer::Print(QueryPlaceAnalysisContext& context) {
        std::cout   << "\nNET INFO:\n"
                    << "Number of places: " << parent->numberOfPlaces()
                    << " (Originally " << parent->originalNumberOfPlaces() << ")" << std::endl
                    << "Number of transitions: " << parent->numberOfTransitions()
                    << " (Originally " << parent->originalNumberOfTransitions() << ")"
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

    uint32_t Reducer::numberOfUnskippedTransitions()
    {
        return parent->numberOfTransitions() - numberOfSkippedTransitions();
    }

    uint32_t Reducer::numberOfUnskippedPlaces()
    {
        return parent->numberOfPlaces() - numberOfSkippedPlaces();
    }

    int32_t Reducer::removedTransitions()
    {
        // Can be negative if transitions was added during reduction
        return (int32_t)parent->_originalNumberOfTransitions - (int32_t)numberOfUnskippedTransitions();
    }

    int32_t Reducer::removedPlaces()
    {
        return (int32_t)parent->_originalNumberOfPlaces - (int32_t)numberOfUnskippedPlaces();
    }

    const shared_const_string& Reducer::getTransitionName(uint32_t transition) const
    {
        for(auto& t : parent->_transitionnames)
        {
            if(t.second == transition) return t.first;
        }
        throw base_error("Unknown transition_id=", transition);
    }

    shared_const_string Reducer::newTransName()
    {
        auto prefix = "CT";
        auto tmp = std::make_shared<const_string>(prefix + std::to_string(_tnameid));
        while(parent->_transitionnames.count(tmp) >= 1)
        {
            ++_tnameid;
            tmp = std::make_shared<const_string>(prefix + std::to_string(_tnameid));
        }
        ++_tnameid;
        return tmp;
    }

    const shared_const_string& Reducer::getPlaceName(uint32_t place) const
    {
        for(auto& t : parent->_placenames)
        {
            if(t.second == place) return t.first;
        }
        throw base_error("Unknown placeid=", place);
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
        _skippedTransitions.push_back(t);
        assert(consistent());
    }

    void Reducer::skipPlace(uint32_t place)
    {
        ++_skippedPlaces;
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

    void Reducer::skipInArc(uint32_t p, uint32_t t)
    {
        Place& place = parent->_places[p];
        Transition& trans = parent->_transitions[t];

        eraseTransition(place.consumers, t);

        Arc a;
        a.place = p;
        auto ait = std::lower_bound(trans.pre.begin(), trans.pre.end(), a);
        assert(ait != trans.pre.end());
        trans.pre.erase(ait);
        assert(consistent());
    }

    void Reducer::skipOutArc(uint32_t t, uint32_t p)
    {
        Place& place = parent->_places[p];
        Transition& trans = parent->_transitions[t];

        eraseTransition(place.producers, t);

        Arc a;
        a.place = p;
        auto ait = std::lower_bound(trans.post.begin(), trans.post.end(), a);
        assert(ait != trans.post.end());
        trans.post.erase(ait);
        assert(consistent());
    }

    bool Reducer::consistent()
    {
#ifndef NDEBUG
        size_t strans = 0;
        for(size_t i = 0; i < parent->numberOfTransitions(); ++i)
        {
            Transition& t = parent->_transitions[i];
            assert(!t.skip || std::find(_skippedTransitions.begin(), _skippedTransitions.end(), i) != _skippedTransitions.end());
            assert(std::is_sorted(t.pre.begin(), t.pre.end()));
            assert(std::is_sorted(t.post.end(), t.post.end()));
            assert(!t.skip || (t.pre.size() == 0 && t.post.size() == 0));
            if (t.skip) strans++;
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

        assert(strans == _skippedTransitions.size());

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
        assert(splaces == _skippedPlaces);
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
            auto w = trans.pre[0].weight;
            bool ok = true;
            for(auto t : parent->_places[pPre].producers)
            {
                if((getOutArc(parent->_transitions[t], trans.pre[0].place)->weight % w) != 0)
                {
                    ok = false;
                    break;
                }
            }
            if(!ok)
                continue;

            // A4. Do inhibitor check, neither T, pPre or pPost can be involved with any inhibitor
            if(parent->_places[pPre].inhib || trans.inhib) continue;

            // A5. dont mess with query!
            if(placeInQuery[pPre] > 0) continue;
            // check A1, A4 and A5 for post
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
                auto tname = getTransitionName(t);
                for(size_t pp : pre.producers)
                {
                    auto prefire = getTransitionName(pp);
                    _postfire[*prefire].push_back(tname);
                }
                for(size_t i = 0; i < parent->initMarking()[pPre]; ++i)
                {
                    _initfire.push_back(tname);
                }
            }

            for(auto& pPost : trans.post)
            {
                // UA2. move the token for the initial marking, makes things simpler.
                parent->initialMarking[pPost.place] += ((parent->initialMarking[pPre]/w) * pPost.weight);
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
                    a.weight = (source.weight/w) * pPost.weight;
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
                        dest->weight += ((source.weight/w) * pPost.weight);
                    }
                    assert(dest->weight > 0);
                }
            }
            // UA1. remove place
            skipPlace(pPre);
        } // end of Rule A main for-loop
        return continueReductions;
    }

    bool Reducer::ReducebyRuleB(uint32_t* placeInQuery, bool remove_deadlocks, bool remove_consumers) {

        // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();
        for (uint32_t p = 0; p < numberofplaces; p++) {
            if(hasTimedout()) return false;
            Place& place = parent->_places[p];

            if(place.skip) continue;    // already removed
            // B5. dont mess up query
            if(placeInQuery[p] > 0)
                continue;

            // B2. Only one consumer/producer
            if( place.consumers.size() != 1 ||
                place.producers.size() < 1)
                continue; // no orphan removal

            auto tIn = place.consumers[0];

            // B1. producer is not consumer
            bool ok = true;
            for(auto& tOut : place.producers)
            {
                if (tOut == tIn)
                {
                    ok = false;
                    continue; // cannot remove this kind either
                }
            }
            if(!ok)
                continue;
            auto prod = place.producers;
            Transition& in = getTransition(tIn);
            for(auto tOut : prod)
            {
                Transition& out = getTransition(tOut);

                if(out.post.size() != 1 && in.pre.size() != 1)
                    continue; // at least one has to be singular for this to work

                if((!remove_deadlocks || !remove_consumers) && in.pre.size() != 1)
                    // the buffer can mean deadlocks and other interesting things
                    // also we can "hide" tokens, so we need to make sure not
                    // to remove consumers.
                    continue;

                if(parent->initMarking()[p] > 0 && in.pre.size() != 1)
                    continue;

                auto inArc = getInArc(p, in);
                auto outArc = getOutArc(out, p);

                // B3. Output is a multiple of input and nonzero.
                if(outArc->weight < inArc->weight)
                    continue;
                if((outArc->weight % inArc->weight) != 0)
                    continue;

                size_t multiplier = outArc->weight / inArc->weight;

                // B4. Do inhibitor check, neither In, out or place can be involved with any inhibitor
                if(place.inhib || in.inhib || out.inhib)
                    continue;

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
                    if(post_ok)
                        continue;
                }
                {
                    bool pre_ok = false;
                    for(const Arc& a : in.pre)
                    {
                        pre_ok |= parent->_places[a.place].inhib;
                        pre_ok |= placeInQuery[a.place];
                        if(pre_ok) break;
                    }
                    if(pre_ok)
                        continue;
                }

                bool ok = true;
                if(in.pre.size() > 1)
                    for(const Arc& arc : out.pre)
                        ok &= placeInQuery[arc.place] == 0;
                if(!ok)
                    continue;

                // B2.a Check that there is no other place than p that gives to tPost,
                // tPre can give to other places
                auto& arcs = in.pre.size() < out.post.size() ? in.pre : out.post;
                for (auto& arc : arcs) {
                    if (arc.weight > 0 && arc.place != p) {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                // UB2. we need to remember initial marking
                uint initm = parent->initMarking()[p];
                initm /= inArc->weight; // integer-devision is floor by default

                continueReductions = true;
                _ruleB++;
                if(reconstructTrace)
                {
                    auto toutname = getTransitionName(tOut);
                    auto tinname  = getTransitionName(tIn);
                    for(size_t i = 0; i < multiplier; ++i)
                    {
                        _postfire[*toutname].push_back(tinname);
                    }

                    for(size_t i = 0; initm > 0 && i < initm / inArc->weight; ++i )
                    {
                        _initfire.push_back(tinname);
                    }
                }

                 // UB1. Remove place p
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
                for (auto& arc : in.pre) { // remove tPost
                    if(arc.place == p)
                        continue;
                    auto _arc = getInArc(arc.place, out);
                    // UB2. Update initial marking
                    parent->initialMarking[arc.place] += initm*arc.weight;
                    if(_arc != out.pre.end())
                    {
                        _arc->weight += arc.weight*multiplier;
                    }
                    else
                    {
                        out.pre.push_back(arc);
                        out.pre.back().weight *= multiplier;
                        parent->_places[arc.place].consumers.push_back(tOut);

                        std::sort(out.pre.begin(), out.pre.end());
                        std::sort(parent->_places[arc.place].consumers.begin(),
                                  parent->_places[arc.place].consumers.end());
                    }
                }

                for(auto it = out.post.begin(); it != out.post.end(); ++it)
                {
                    if(it->place == p)
                    {
                        out.post.erase(it);
                        break;
                    }
                }
                for(auto it = place.producers.begin(); it != place.producers.end(); ++it)
                {
                    if(*it == tOut)
                    {
                        place.producers.erase(it);
                        break;
                    }
                }
            }
            // UB1. remove transition
            if(place.producers.size() == 0)
            {
                skipPlace(p);
                skipTransition(tIn);
            }
        } // end of Rule B main for-loop
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleC(uint32_t* placeInQuery) {
        // Rule C - Places in parallel where one accumulates tokens while the others disable their post set
        bool continueReductions = false;
        _pflags.resize(parent->_places.size(), 0);
        std::fill(_pflags.begin(), _pflags.end(), 0);

        for (uint32_t tid_outer = 0; tid_outer < parent->numberOfTransitions(); ++tid_outer) {
            for (size_t aid_outer = 0; aid_outer < parent->_transitions[tid_outer].post.size(); ++aid_outer) {

                auto pid_outer = parent->_transitions[tid_outer].post[aid_outer].place;
                if (_pflags[pid_outer] > 0) continue;
                _pflags[pid_outer] = 1;

                if (hasTimedout()) return false;

                const Place &pout = parent->_places[pid_outer];
                if (pout.skip) continue;

                for (size_t aid_inner = aid_outer + 1; aid_inner < parent->_transitions[tid_outer].post.size(); ++aid_inner) {
                    if (pout.skip) break;
                    auto pid_inner = parent->_transitions[tid_outer].post[aid_inner].place;
                    if (parent->_places[pid_inner].skip) continue;

                    for (size_t swp = 0; swp < 2; ++swp) {
                        if (hasTimedout()) return false;
                        if (parent->_places[pid_inner].skip ||
                            parent->_places[pid_outer].skip)
                            break;

                        uint p1 = pid_outer;
                        uint p2 = pid_inner;

                        assert(p1 != p2);
                        if (swp == 1) std::swap(p1, p2);

                        if (placeInQuery[p2] > 0) continue;

                        Place &place1 = parent->_places[p1];
                        Place &place2 = parent->_places[p2];

                        if (place2.inhib) continue;
                        if (place2.producers.empty() || place1.consumers.empty()) continue;

                        if (place1.consumers.size() < place2.consumers.size() ||
                            place1.producers.size() > place2.producers.size())
                            continue;

                        bool ok = true;

                        double maxDrainRatio = 0;

                        uint32_t i = 0, j = 0;
                        while (i < place1.consumers.size() && j < place2.consumers.size()) {

                            uint32_t p1t = place1.consumers[i];
                            uint32_t p2t = place2.consumers[j];

                            if (p2t < p1t) {
                                // place2.consumers is not a subset of place1.consumers
                                ok = false;
                                break;
                            }

                            i++;
                            if (p2t > p1t) {
                                swp = 2; // We can't remove p1, so don't swap
                                continue;
                            }
                            j++;

                            Transition &tran = getTransition(p1t);
                            const auto &p1Arc = getInArc(p1, tran);
                            const auto &p2Arc = getInArc(p2, tran);

                            maxDrainRatio = std::max(maxDrainRatio, (double)p2Arc->weight / (double)p1Arc->weight);
                        }

                        if (!ok || j != place2.consumers.size()) continue;

                        if (parent->initialMarking[p2] < parent->initialMarking[p1] * maxDrainRatio) continue;

                        i = 0, j = 0;
                        while (i < place1.producers.size() && j < place2.producers.size()) {

                            uint32_t p1t = place1.producers[i];
                            uint32_t p2t = place2.producers[j];

                            if (p1t < p2t) {
                                // place1.producers is not a subset of place2.producers
                                ok = false;
                                break;
                            }

                            j++;
                            if (p1t > p2t) {
                                swp = 2; // We can't remove p1, so don't swap
                                continue;
                            }
                            i++;

                            Transition &tran = getTransition(p2t);
                            const auto &p2Arc = getOutArc(tran, p2);
                            const auto &p1Arc = getOutArc(tran, p1);

                            if (maxDrainRatio > (double)p2Arc->weight / (double)p1Arc->weight) {
                                ok = false;
                                break;
                            }
                        }

                        if (!ok || i != place1.producers.size()) continue;

                        continueReductions = true;
                        _ruleC++;

                        skipPlace(p2);

                        // p2 has now been removed from tid_outer.post, so update arc indexes to not miss any places
                        if (p2 == pid_outer) {
                            aid_outer--;
                            aid_inner--;
                        } else if (p2 == pid_inner) aid_inner--;
                        break;
                    }
                }
            }
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleD(uint32_t* placeInQuery, bool all_reach, bool remove_loops_no_branch) {
        // Rule D - two transitions with the same pre and post and same inhibitor arcs
        // This does not alter the trace.
        bool continueReductions = false;
        _tflags.resize(parent->_transitions.size(), 0);
        std::fill(_tflags.begin(), _tflags.end(), 0);
        bool has_empty_trans = false;
        for(size_t t = 0; t < parent->_transitions.size(); ++t)
        {
            auto& trans = parent->_transitions[t];
            if(!trans.skip && trans.pre.size() == 0 && trans.post.size() == 0)
            {
                if(has_empty_trans)
                {
                    ++_ruleD;
                    skipTransition(t);
                }
                has_empty_trans = true;
            }

        }

        for(auto& op : parent->_places)
        for(size_t outer = 0; outer < op.consumers.size(); ++outer)
        {
            auto touter = op.consumers[outer];
            if(hasTimedout()) return false;
            if(_tflags[touter] != 0) continue;
            _tflags[touter] = 1;
            Transition& tout = getTransition(touter);
            if (tout.skip) continue;

            // D2. No inhibitors
            if (tout.inhib) continue;

            for(size_t inner = outer + 1; inner < op.consumers.size(); ++inner) {
                if (tout.skip) break;
                auto tinner = op.consumers[inner];
                Transition& tin = getTransition(tinner);
                if (tin.skip) continue;

                // D2. No inhibitors
                if (tin.inhib) continue;

                for (size_t swp = 0; swp < 2; ++swp) {
                    if(hasTimedout()) return false;

                    if (tin.skip || tout.skip) break;

                    uint t1 = touter;
                    uint t2 = tinner;
                    if (swp == 1) std::swap(t1, t2);

                    // D1. not same transition
                    assert(t1 != t2);

                    Transition& trans1 = getTransition(t1);
                    Transition& trans2 = getTransition(t2);

                    // From D3, and D4 we have that pre and post-sets are the same
                    if (trans1.post.size() < trans2.post.size()) { break;}
                    if (trans1.pre.size() > trans2.pre.size()) { break;}
                    if (!remove_loops_no_branch && (trans1.pre.size() != trans2.pre.size() ||
                                          trans1.post.size() != trans2.post.size()))
                    {
                        break; // we require exactness.
                    }

                    int ok = 0;
                    uint mult = std::numeric_limits<uint>::max();
                    bool pre_equal = true;
                    bool post_equal = true;
                    bool some_in_query = false;
                    bool exact = true;
                    // D3. Presets must match
                    size_t j = 0;
                    for (size_t i = 0; i < trans1.pre.size(); ++i, ++j) {
                        Arc& arc = trans1.pre[i];
                        for(; j < trans2.pre.size() && trans2.pre[j].place < arc.place; ++j)
                        {
                            pre_equal &= placeInQuery[trans2.pre[j].place] == 0;
                            exact = false;
                        }

                        if(j >= trans2.pre.size() || trans2.pre[j].place != arc.place)
                        {
                            ok = 1;
                            break;
                        }

                        Arc& arc2 = trans2.pre[j];
                        if (arc2.place != arc.place) {
                            ok = 2;
                            break;
                        }
                        if (arc2.weight < arc.weight) {
                            ok = 1;
                            break;
                        } else {
                            auto old = mult;
                            mult = std::min(arc2.weight / arc.weight, mult);
                            if(old != std::numeric_limits<uint>::max() &&
                               mult != old &&
                               some_in_query)
                            {
                                pre_equal = false;
                                exact = false;
                            }
                        }
                        some_in_query |= placeInQuery[arc2.place] > 0;
                        pre_equal = pre_equal && (placeInQuery[arc2.place] == 0 ||
                                                  arc.weight == arc2.weight*mult);
                        exact &= arc.weight == arc2.weight*mult;
                        if(!pre_equal) break;
                    }
                    for(; j < trans2.pre.size(); ++j)
                    {
                        pre_equal &= placeInQuery[trans2.pre[j].place] == 0;
                        exact = false;
                    }

                    if(!pre_equal) { break;}
                    if (ok == 2) { break;}
                    else if (ok == 1) { continue;}
                    if(mult != 1 && !all_reach) { break;}
                    if(!remove_loops_no_branch && !exact) { break;}
                    ok = 0;
                    // D4. postsets must match
                    j = 0;
                    for (size_t i = 0; i < trans2.post.size(); ++i, ++j) {
                        Arc& arc2 = trans2.post[i];
                        for(; j < trans1.post.size() && trans1.post[j].place < arc2.place; ++j)
                        {
                            post_equal &= placeInQuery[trans1.post[j].place] == 0;
                            exact = false;
                        }
                        if(j >= trans1.post.size() || trans1.post[j].place != arc2.place)
                        {
                            ok = 1;
                            break;
                        }
                        Arc& arc = trans1.post[j];
                        if (arc2.place != arc.place) {
                            ok = 2;
                            break;
                        }
                        post_equal = post_equal && (placeInQuery[arc2.place] == 0 ||
                                                    arc.weight * mult == arc2.weight);
                        exact &= arc.weight * mult == arc2.weight;
                        if (arc2.weight > arc.weight * mult) {
                            ok = 2;
                            break;
                        }
                        if(!post_equal) break;
                    }

                    for(; j < trans1.post.size(); ++j)
                    {
                        post_equal &= placeInQuery[trans1.post[j].place] == 0;
                        exact = false;
                    }

                    if(!post_equal) { break;}
                    if (ok == 2) { break;}
                    else if (ok == 1) {
                        continue;
                    }
                    if(!remove_loops_no_branch && !exact) { break;}

                    // UD1. Remove transition t2
                    continueReductions = true;
                    _ruleD++;
                    skipTransition(t2);
                    _tflags[touter] = 0;

                    // t2 has now been removed from op.consumers, so update indexes to not miss any
                    if (t2 == touter) {
                        outer--;
                        inner--;
                    } else if (t2 == tinner) inner--;
                    break; // break the swap loop
                }
            }
        } // end of main for loop for rule D
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleEP(uint32_t* placeInQuery) {
        // Rule P is an extension on Rule E
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();
        for(uint32_t p = 0; p < numberofplaces; ++p)
        {
            if(hasTimedout()) return false;
            Place& place = parent->_places[p];
            if(place.skip) continue;
            // If more producers, we are guaranteed that one producer have a positive effect on the place, and as such E1 precondition is false
            if(place.producers.size() > place.consumers.size()) continue;

            bool ok = true;

            // Check for producers without matching consumers first
            for(uint prod : place.producers)
            {
                // Any producer without a matching consumer blocks this rule
                Transition& t = getTransition(prod);
                const auto& in = getInArc(p, t);
                if(in == t.pre.end() || in->inhib)
                {
                    ok = false;
                    break;
                }
            }

            if(!ok) continue;
            std::unordered_set<uint32_t> initiallyDisabled;
            std::unordered_set<uint32_t> inhibited;
            // Collect transitions initially disabled by the place and transitions with inactive inhibitor arcs.
            // Abort if we find an enabled transition with positive effect on the place
            for(uint cons : place.consumers)
            {
                Transition& t = getTransition(cons);
                const auto& in = getInArc(p, t);
                if (!in->inhib) {
                    if (in->weight <= parent->initialMarking[p]) {
                        // Check if positive effect on place. If positive, abort.
                        const auto& out = getOutArc(t, p);
                        if (out != t.post.end() && out->weight > in->weight) {
                            ok = false;
                            break;
                        }
                    } else {
                        initiallyDisabled.insert(cons);
                    }
                } else {
                    if (in->weight > parent->initialMarking[p]) {
                        // Inactive inhibitor arc
                        inhibited.insert(cons);
                    }
                }
            }

            if(!ok || (initiallyDisabled.empty() && inhibited.empty())) continue;

            if (!initiallyDisabled.empty()) _ruleE++;
            if (!inhibited.empty()) _ruleP++;
            continueReductions = true;  // Either rule E or P is for sure enabled at this point

            bool allArcsRemoved = initiallyDisabled.size() + inhibited.size() == place.consumers.size();

            // Can the place be removed entirely?
            if(allArcsRemoved && (placeInQuery[p] == 0)) {
                skipPlace(p);
                continue;
            }

            for(uint32_t cons : initiallyDisabled) {
                Transition &t = getTransition(cons);
                const auto& in = getInArc(p, t);
                assert(!in->inhib);
                skipTransition(cons);
            }

            for (uint32_t tran : inhibited) {
                Transition &t = getTransition(tran);
                const auto& in = getInArc(p, t);
                assert(in->inhib);
                assert(in->weight > parent->initialMarking[p]);
                skipInArc(p, tran);
            }
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleI(uint32_t* placeInQuery, bool remove_consumers) {
        bool reduced = false;

        auto result = relevant(placeInQuery, remove_consumers);
        if (!result) {
            return false;
        }
        auto[tseen, pseen] = result.value();

        reduced |= remove_irrelevant(placeInQuery, tseen, pseen);

        if(reduced)
            ++_ruleI;

        return reduced;
    }

    bool Reducer::ReducebyRuleF(uint32_t* placeInQuery) {
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
            for(uint32_t cons : place.consumers)
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

            ++_ruleF;

            if((numberofplaces - _skippedPlaces) > 1)
            {
                skipPlace(p);
                continueReductions = true;
            }

        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleFNO(uint32_t* placeInQuery) {
        // Redundant arc (and place) removal.
        // If a place p never disables a transition, we can remove its arc to the
        // transitions as long as the effect is maintained (Rule N). Similarly, we can remove
        // transitions that are always inhibited (Rule O). If all arcs to a place is removed,
        // then we remove the place too (Rule F).
        bool continueReductions = false;
        const size_t numberofplaces = parent->numberOfPlaces();

        for (uint32_t p = 0; p < numberofplaces; ++p)
        {
            if (hasTimedout()) return false;
            Place& place = parent->_places[p];
            if (place.skip) continue;

            bool removePlace = placeInQuery[p] == 0;

            // Use tflags to mark producers with negative effect
            _tflags.resize(parent->_transitions.size(), 0);
            std::fill(_tflags.begin(), _tflags.end(), 0);

            // Assume all consumers are disableable and non-negative until proven otherwise. Used to apply F.
            uint32_t disableableNonNegative = place.consumers.size();

            uint32_t inhibArcs = 0;

            uint32_t low = parent->initialMarking[p];

            for (uint32_t cons : place.consumers)
            {
                Transition& tran = getTransition(cons);
                const auto & inArc = getInArc(p, tran);

                if (inArc->inhib)
                {
                    inhibArcs++;
                    continue;
                }

                const auto & outArc = getOutArc(tran, p);

                if (outArc != tran.post.end()) {

                    uint32_t outArcWeight = outArc->weight;
                    uint32_t inArcWeight = inArc->weight;

                    if (outArcWeight < inArcWeight)
                    {
                        // This transition is a consumer with negative effect
                        disableableNonNegative -= 1;
                        _tflags[cons] = 1;
                        removePlace = false;

                        if (outArcWeight < low)
                        {
                            // We found a new lower bound
                            low = outArcWeight;
                        }
                    }
                }
                else
                {
                    low = 0;
                    break;
                }
            }

            // Consumer arcs exists, but none will have a weight lower than 0, so we cannot reduce
            if (!place.consumers.empty() && low == 0) continue;

            std::set<uint32_t> alwaysInhibited;

            // Copy of the vector to iterate over while removing from the original
            const std::vector<uint32_t> consumers = place.consumers;
            for (uint32_t cons : consumers)
            {
                if (_tflags[cons] == 1) continue;

                Transition& tran = getTransition(cons);
                const auto & inArc = getInArc(p, tran);

                if (inArc->weight <= low)
                {
                    if (inArc->inhib)
                    {
                        // This transition is always disabled by p
                        alwaysInhibited.insert(cons);
                        continueReductions = true;
                    }
                    else
                    {
                        // This consumer is never disabled by p, so we can remove its arc (Rule N)
                        const auto & outArc = getOutArc(tran, p);
                        if (inArc->weight == outArc->weight)
                        {
                            skipOutArc(cons, p);
                        }
                        else
                        {
                            outArc->weight -= inArc->weight;
                        }
                        skipInArc(p, cons);

                        disableableNonNegative -= 1;
                        continueReductions = true;
                        _ruleN += 1;
                    }
                }
}

// Apply rule O
            inhibArcs -= alwaysInhibited.size();
            _ruleO += alwaysInhibited.size();

            for (const auto & inhibited : alwaysInhibited)
                skipTransition(inhibited);

            // Apply rule F
            if (removePlace && inhibArcs == 0 && disableableNonNegative == 0 && numberofplaces - _skippedPlaces > 1)
            {
                skipPlace(p);
                continueReductions = true;
                _ruleF++;
            }
            else if (inhibArcs == 0)
            {
                place.inhib = false;
            }
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleG(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers) {
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
                if(preit->inhib || parent->_places[preit->place].inhib)
                {
                    ok = false;
                    break;
                }
                if(postit != trans.post.end() && preit->place == postit->place)
                {
                    if(!remove_consumers && preit->weight != postit->weight)
                    {
                        ok = false;
                        break;
                    }
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
                    if(placeInQuery[preit->place] > 0 || !remove_consumers)
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
            if(ok)
            {
                for(preit = trans.pre.begin();preit != trans.pre.end(); ++preit)
                {
                    if(preit->inhib || parent->_places[preit->place].inhib)
                    {
                        ok = false;
                        break;
                    }
                }
            }

            if(!ok) continue;
            ++_ruleG;
            skipTransition(t);
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::ReducebyRuleH(uint32_t* placeInQuery, bool all_ltl)
    {
        if(reconstructTrace)
            return false; // we don't know where in the loop the tokens are needed
        auto transok = [this](uint32_t t) -> uint32_t {
            auto& trans = parent->_transitions[t];
            if(_tflags[t] != 0)
                return _tflags[t];
            _tflags[t] = 1;
            if(trans.inhib ||
               trans.pre.size() != 1 ||
               trans.post.size() != 1)
            {
                return 2;
            }

            auto p1 = trans.pre[0].place;
            auto p2 = trans.post[0].place;

            // we actually do not need weights to be 1 here.
            // there is a special case when the places are always "inputting"
            // and "outputting" with a GCD that is equal to the weight of the
            // specific transition.
            // Ie, the place always have a number of tokens (disregarding
            // initial tokens) that is dividable with the transition weight

            if(trans.pre[0].weight != 1 ||
               trans.post[0].weight != 1 ||
               p1 == p2 ||
               parent->_places[p1].inhib ||
               parent->_places[p2].inhib)
            {
                return 2;
            }
            return 1;
        };

        auto removeLoop = [this,placeInQuery,all_ltl](std::vector<uint32_t>& loop) -> bool {
            size_t i = 0;
            for(; i < loop.size(); ++i)
                if(loop[i] == loop.back())
                    break;

            assert(_tflags[loop.back()]== 1);
            if(i == loop.size() - 1)
                return false;

            auto p1 = parent->_transitions[loop[i]].pre[0].place;
            bool removed = false;

            for(size_t j = i + 1; j < loop.size() - 1; ++j)
            {
                if(hasTimedout())
                    return removed;
                auto p2 = parent->_transitions[loop[j]].pre[0].place;
                if(placeInQuery[p2] > 0 || placeInQuery[p1] > 0)
                {
                    p1 = p2;
                    continue;
                }
                if(p1 == p2)
                {
                    continue;
                }

                if(!all_ltl)
                {
                    bool ok = true;
                    for(auto p : {p1, p2})
                    {
                        for(auto& a : {parent->_places[p].producers, parent->_places[p].consumers})
                        {
                            for(auto& t : a)
                            {
                                auto& trans = getTransition(t);
                                for(auto& arcset : {trans.pre, trans.post})
                                {
                                    for(auto& arc : arcset)
                                    {
                                        if(placeInQuery[arc.place] > 0)
                                        {
                                            ok = false;
                                            break;
                                        }
                                    }
                                    if(!ok) break;
                                }
                                if(!ok) break;
                            }
                            if(!ok) break;
                        }
                        if(!ok) break;
                    }
                    if(!ok) continue;
                }

                removed = true;
                ++_ruleH;
                skipTransition(loop[j-1]);
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
                    for(auto p2it : place2.producers)
                    {
                        auto& t = parent->_transitions[p2it];
                        Arc a = *getOutArc(t, p2);
                        a.place = p1;
                        auto dest = std::lower_bound(t.post.begin(), t.post.end(), a);
                        if(dest == t.post.end() || dest->place != p1)
                        {
                            t.post.insert(dest, a);
                            auto lb = std::lower_bound(place1.producers.begin(), place1.producers.end(), p2it);
                            place1.producers.insert(lb, p2it);
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
                assert(placeInQuery[p2] == 0);
            }
            return removed;
        };

        bool continueReductions = false;
        for(uint32_t t = 0; t < parent->numberOfTransitions(); ++t)
        {
            if(hasTimedout())
                return false;
            _tflags.resize(parent->_transitions.size(), 0);
            std::fill(_tflags.begin(), _tflags.end(), 0);
            std::vector<uint32_t> stack;
            {
                if(_tflags[t] != 0) continue;
                auto& trans = parent->_transitions[t];
                if(trans.skip) continue;
                _tflags[t] = transok(t);
                if(_tflags[t] != 1) continue;
                stack.push_back(t);
            }
            bool outer = true;
            while(stack.size() > 0 && outer)
            {
                if(hasTimedout())
                    return continueReductions;
                auto it = stack.back();
                auto post = parent->_transitions[it].post[0].place;
                bool found = false;
                for(auto& nt : parent->_places[post].consumers)
                {
                    if(hasTimedout())
                        return continueReductions;
                    auto& nexttrans = parent->_transitions[nt];
                    if(nt == it || nexttrans.skip)
                        continue; // handled elsewhere
                    if(_tflags[nt] == 1 && stack.size() > 1)
                    {
                        stack.push_back(nt);
                        bool found = removeLoop(stack);
                        continueReductions |= found;

                        if(found)
                        {
                            outer = false;
                            break;
                        }
                        else
                        {
                            stack.pop_back();
                        }
                    }
                    else if(_tflags[nt] == 0)
                    {
                        _tflags[nt] = transok(nt);
                        if(_tflags[nt] == 2)
                        {
                            continue;
                        }
                        else
                        {
                            assert(_tflags[nt] == 1);
                            stack.push_back(nt);
                            found = true;
                            break;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                if(!found && outer)
                {
                    _tflags[it] = 2;
                    stack.pop_back();
                }
            }
        }
        return continueReductions;
    }

    bool Reducer::ReducebyRuleJ(uint32_t* placeInQuery) {
        if(reconstructTrace)
            return false;
        bool any = false;
        for(std::size_t p = 0; p < parent->numberOfPlaces(); ++p)
        {
            if(placeInQuery[p] > 0) continue;
            auto& place = parent->_places[p];
            if(place.skip) continue;
            if(place.consumers.empty() && place.producers.empty())
                continue;
            uint32_t mod = std::numeric_limits<uint32_t>::max();
            bool ok = true;
            for(auto& t : parent->_places[p].consumers)
            {
                auto& trans = getTransition(t);
                auto arc = getInArc(p, trans);
                if(mod == std::numeric_limits<uint32_t>::max())
                    mod = arc->weight;
                else
                    mod = std::gcd(mod, arc->weight);
                if(mod == 1)
                    break;
            }
            if(!ok) continue;
            if(mod == 1) continue;
            for(auto& t : parent->_places[p].producers)
            {
                auto& trans = getTransition(t);
                auto arc = getOutArc(trans, p);
                if(mod == std::numeric_limits<uint32_t>::max())
                    mod = arc->weight;
                else
                    mod = std::gcd(mod, arc->weight);
                if(mod == 1)
                    break;
            }
            if(!ok) continue;
            if(mod == 1) continue;

            ++_ruleJ;

            for(auto& t : parent->_places[p].consumers)
            {
                auto& trans = getTransition(t);
                auto arc = getInArc(p, trans);
                arc->weight /= mod;
            }
            for(auto& t : parent->_places[p].producers)
            {
                auto& trans = getTransition(t);
                auto arc = getOutArc(trans, p);
                arc->weight /= mod;
            }
            parent->initialMarking[p] /= mod;
            any = true;
        }
        return any;
    }

    bool Reducer::ReducebyRuleK(uint32_t *placeInQuery, bool remove_consumers) {
        bool reduced = false;
        auto opt = relevant(placeInQuery, remove_consumers);
        if (!opt)
            return false;
        auto[tseen, pseen] = opt.value();
        for (std::size_t p = 0; p < parent->numberOfPlaces(); ++p) {
            if (placeInQuery[p] != 0)
                pseen[p] = true;
        }

        for (std::size_t t = 0; t < parent->numberOfTransitions(); ++t) {
            auto transition = parent->_transitions[t];
            if (!tseen[t] && !transition.skip && !transition.inhib && transition.pre.size() == 1 &&
                    // Search for simple self loops outside the relevant area that could be kept to preserve liveness?
                    transition.post.size() == 1
                    && transition.pre[0].place == transition.post[0].place) {
                auto p = transition.pre[0].place;
                if (!pseen[p] && !parent->_places[p].inhib) {
                    if (parent->initialMarking[p] >= transition.pre[0].weight){
                        // Mark the initially marked self loop as relevant.
                        tseen[t] = true;
                        pseen[p] = true;
                        reduced |= remove_irrelevant(placeInQuery, tseen, pseen);
                        _ruleK++;
                        return reduced;
                    }
                    if (transition.pre[0].weight == 1){
                        // If a single token can enable a self loop, other consumers of that place are irrelevant to liveness
                        for (auto t2 : parent->_places[p].consumers) {
                            auto transition2 = parent->_transitions[t2];
                            if (t != t2 && !tseen[t2] && !transition2.skip) {
                                skipTransition(t2);
                                reduced = true;
                                _ruleK++;
                            }
                        }
                    }
                }
            }
        }
        return reduced;
    }

    std::optional<std::pair<std::vector<bool>, std::vector<bool>>>
    Reducer::relevant(const uint32_t *placeInQuery, bool remove_consumers) {
        std::vector<uint32_t> wtrans;
        std::vector<bool> tseen(parent->numberOfTransitions(), false);
        for (uint32_t p = 0; p < parent->numberOfPlaces(); ++p) {
            if (hasTimedout()) return std::nullopt;
            if (placeInQuery[p] > 0) {
                const Place &place = parent->_places[p];
                for (auto t : place.consumers) {
                    if (!tseen[t]) {
                        wtrans.push_back(t);
                        tseen[t] = true;
                    }
                }
                for (auto t : place.producers) {
                    if (!tseen[t]) {
                        wtrans.push_back(t);
                        tseen[t] = true;
                    }
                }
            }
        }
        std::vector<bool> pseen(parent->numberOfPlaces(), false);

        while (!wtrans.empty()) {
            if (hasTimedout()) return std::nullopt;
            auto t = wtrans.back();
            wtrans.pop_back();
            const Transition &trans = parent->_transitions[t];
            for (const Arc &arc : trans.pre) {
                const Place &place = parent->_places[arc.place];
                if (arc.inhib) {
                    for (auto pt : place.consumers) {
                        if (!tseen[pt]) {
                            // Summary of block: 'pt' is seen unless it:
                            // - Is inhibited by 'place'
                            // - Forms a decreasing loop on 'place' that cannot lower the marking of 'place' below the weight of 'arc'
                            // - Forms a non-decreasing loop on 'place'
                            Transition &trans = parent->_transitions[pt];
                            auto it = trans.post.begin();
                            for (; it != trans.post.end(); ++it)
                                if (it->place >= arc.place) break;

                            if (it != trans.post.end() && it->place == arc.place) {
                                auto it2 = trans.pre.begin();
                                // Find the arc from place to trans we know to exist because that is how we found trans in the first place
                                for (; it2 != trans.pre.end(); ++it2)
                                    if (it2->place >= arc.place) break;
                                // No need for a || it2->place != arc.place condition because we know the loop will always break on it2->place == arc.place
                                if (it2->inhib || it->weight >= arc.weight || it->weight >= it2->weight) continue;
                            }
                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }
                } else {
                    for (auto pt : place.producers) {
                        if (!tseen[pt]) {
                            // Summary of block: pt is seen unless it forms a non-increasing loop on place
                            Transition &trans = parent->_transitions[pt];
                            auto it = trans.pre.begin();
                            for (; it != trans.pre.end(); ++it)
                                if (it->place >= arc.place) break;

                            if (it != trans.pre.end() && it->place == arc.place && !it->inhib) {
                                auto it2 = trans.post.begin();
                                for (; it2 != trans.post.end(); ++it2)
                                    if (it2->place >= arc.place) break;
                                if (it->weight >= it2->weight) continue;
                            }
                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }

                    for (auto pt : place.consumers) {
                        if (!tseen[pt] && (!remove_consumers || placeInQuery[arc.place] > 0)) {
                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }
                }
                pseen[arc.place] = true;
            }
        }
        return std::make_optional(std::pair(tseen, pseen));
    }

    bool Reducer::remove_irrelevant(const uint32_t* placeInQuery, const std::vector<bool> &tseen, const std::vector<bool> &pseen) {
        bool reduced = false;
        for(size_t t = 0; t < parent->numberOfTransitions(); ++t)
        {
            if(!tseen[t] && !parent->_transitions[t].skip)
            {
                skipTransition(t);
                reduced = true;
            }
        }

        for(size_t p = 0; p < parent->numberOfPlaces(); ++p)
        {
            if(!pseen[p] && !parent->_places[p].skip && placeInQuery[p] == 0)
            {
                assert(placeInQuery[p] == 0);
                skipPlace(p);
                reduced = true;
            }
        }
        return reduced;
    }

    bool Reducer::ReducebyRuleL(uint32_t *placeInQuery) {
        // When a transition t1 has the same effect as t2, but more pre conditions,
        // which can happen due to read arc behavior, t1 can be discarded.
        // Rule 2 from "Structural Reductions Revisited" by yann thierry-mieg

        bool continueReductions = false;
        if(parent->numberOfTransitions() == 0)
            return false;
        for (size_t t1 = 0; t1 < parent->numberOfTransitions() - 1; ++t1) {
            Transition &tran1 = getTransition(t1);
            if (tran1.skip) break;
            for (const auto & t1i : tran1.pre){
                Place &p = parent->_places[t1i.place];
                for (uint32_t t2 : p.consumers){
                    if (hasTimedout()) return false;

                    if (t2 == t1) continue;

                    Transition &tran2 = getTransition(t2);
                    if (tran2.skip) continue;

                    // We try to remove t1 despite it being the outer loop,
                    // because p.consumers will be invalidated anyway regardless of which one we remove.
                    bool canT1BeRemoved = tran1.pre.size() >= tran2.pre.size() && tran1.post.size() >= tran2.post.size();

                    if (!(canT1BeRemoved)) {
                        continue;
                    }

                    // get arcs
                    const std::vector<Arc> & pre_subset = tran2.pre;
                    const std::vector<Arc> & pre_superset = tran1.pre;
                    const std::vector<Arc> & post_subset = tran2.post;
                    const std::vector<Arc> & post_superset = tran1.post;

                    bool ok = true;
                    uint32_t i = 0, j = 0, k = 0, l = 0;
                    bool i_done = i >= pre_subset.size(), j_done = j >= pre_superset.size(), k_done = k >= post_subset.size(), l_done = l >= post_superset.size();
                    // Taking advantage of the pre- and post-sets being ordered by place_id
                    while (!i_done || !j_done || !k_done || !l_done) {
                        if (hasTimedout()) return false;
                        // The lowest place_id from a set that is not done yet
                        auto place = std::min({i_done ? std::numeric_limits<uint32_t>::max() : pre_subset[i].place,
                                               j_done ? std::numeric_limits<uint32_t>::max() : pre_superset[j].place,
                                               k_done ? std::numeric_limits<uint32_t>::max() : post_subset[k].place,
                                               l_done ? std::numeric_limits<uint32_t>::max() : post_superset[l].place});

                        // Precondition stuff
                        if (!i_done && place == pre_subset[i].place) {

                            // Make sure everything in pre_subset is in pre_superset
                            if (j_done || pre_superset[j].place != place) {
                                ok = false;
                                break;
                            }
                                // Same place, check preconditions
                            else if (pre_subset[i].place == pre_superset[j].place) {
                                // Inhibitor check
                                if (pre_subset[i].inhib != pre_superset[j].inhib){
                                    ok = false;
                                    break;
                                }

                                // Check the requirement to fire. For non-inhibitors i <= j should hold, for inhibitors it is i >= j
                                if ((!pre_subset[i].inhib && pre_superset[j].weight < pre_subset[i].weight) ||
                                     (pre_subset[i].inhib && pre_superset[j].weight > pre_subset[i].weight)) {
                                    ok = false;
                                    break;
                                }
                            }
                        }

                        // Effect stuff
                        size_t subset_in_weight = 0;
                        if (!i_done && place == pre_subset[i].place){
                            if (!pre_subset[i].inhib) subset_in_weight = pre_subset[i].weight;
                            i++;
                            i_done = i >= pre_subset.size();
                        }
                        size_t superset_in_weight = 0;
                        if (!j_done && place == pre_superset[j].place){
                            if (!pre_superset[j].inhib) superset_in_weight = pre_superset[j].weight;
                            j++;
                            j_done = j >= pre_superset.size();
                        }
                        size_t subset_out_weight = 0;
                        if (!k_done && place == post_subset[k].place){
                            if (!post_subset[k].inhib) subset_out_weight = post_subset[k].weight;
                            k++;
                            k_done = k >= post_subset.size();
                        }
                        size_t superset_out_weight = 0;
                        if (!l_done && place == post_superset[l].place){
                            if (!post_superset[l].inhib) superset_out_weight = post_superset[l].weight;
                            l++;
                            l_done = l >= post_superset.size();
                        }

                        if ((superset_out_weight - superset_in_weight) != (subset_out_weight - subset_in_weight)) {
                            ok = false;
                            break;
                        }
                    }

                    // Go to next t2
                    if (!ok) {
                        continue;
                    }

                    // We can discard t1
                    skipTransition(t1);
                    _ruleL++;
                    continueReductions = true;
                    break;
                }
                // If t1 was reduced away, we need to break out further.
                if (tran1.skip) break;
            }
        }

        return continueReductions;
    }

    bool Reducer::ReducebyRuleM(uint32_t* placeInQuery) {
        // Dead places and transitions
        if (hasTimedout()) return false;

        // Use pflags and bits to keep track of places that can increase or decrease their number of tokens
        const uint8_t CAN_INC = 0b01;
        const uint8_t CAN_DEC = 0b10;
        _pflags.resize(parent->_places.size(), 0);
        std::fill(_pflags.begin(), _pflags.end(), 0);

        // Use tflags to mark processed fireable transitions
        _tflags.resize(parent->_transitions.size(), 0);
        std::fill(_tflags.begin(), _tflags.end(), 0);

        // Queue of potentially fireable transitions to process
        std::queue<uint32_t> queue;

        auto processIncPlace = [&](uint32_t p) {
            if ((_pflags[p] & CAN_INC) == 0) {
                _pflags[p] |= CAN_INC;
                Place place = parent->_places[p];
                for (uint32_t t : place.consumers) {
                    if (_tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processDecPlace = [&](uint32_t p) {
            if ((_pflags[p] & CAN_DEC) == 0) {
                _pflags[p] |= CAN_DEC;
                Place place = parent->_places[p];
                for (uint32_t t : place.consumers) {
                    if (_tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processEnabled = [&](uint32_t t) {
            _tflags[t] = 1;
            Transition& tran = parent->_transitions[t];
            // Find and process negative preset and positive postset
            uint32_t i = 0, j = 0;
            while (i < tran.pre.size() && j < tran.post.size())
            {
                if (tran.pre[i].place < tran.post[j].place) {
                    if (!tran.pre[i].inhib)
                        processDecPlace(tran.pre[i].place);
                    i++;
                } else if (tran.pre[i].place > tran.post[j].place) {
                    processIncPlace(tran.post[j].place);
                    j++;
                } else {
                    if (tran.pre[i].inhib) {
                        processIncPlace(tran.post[j].place);
                    } else {
                        // There are both an in and an out arc to this place. Is the effect non-zero?
                        if (tran.pre[i].weight > tran.post[j].weight) {
                            processDecPlace(tran.pre[i].place);
                        } else if (tran.pre[i].weight < tran.post[j].weight) {
                            processIncPlace(tran.post[j].place);
                        }
                    }

                    i++; j++;
                }
            }
            for ( ; i < tran.pre.size(); i++) {
                if (!tran.pre[i].inhib)
                    processDecPlace(tran.pre[i].place);
            }
            for ( ; j < tran.post.size(); j++) {
                processIncPlace(tran.post[j].place);
            }
        };

        // Process initially enabled transitions
        for (uint32_t t = 0; t < parent->_transitions.size(); ++t) {
            Transition& tran = parent->_transitions[t];
            if (tran.skip)
                continue;
            bool enabled = true;
            for (Arc& prearc : tran.pre) {
                if (prearc.inhib != (prearc.weight > parent->initialMarking[prearc.place])) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Now we find the fixed point of S_cant_inc, S_cant_dec, and _tflags iteratively

        while (!queue.empty()) {
            if (hasTimedout()) return false;

            uint32_t t = queue.front();
            queue.pop();
            if (_tflags[t] == 1) continue;

            // Is t enabled?
            bool enabled = true;
            for (Arc prearc : parent->_transitions[t].pre) {
                bool notInhibited = !prearc.inhib || prearc.weight > parent->initialMarking[prearc.place] || (_pflags[prearc.place] & CAN_DEC) > 0;
                bool enoughTokens = prearc.inhib || prearc.weight <= parent->initialMarking[prearc.place] || (_pflags[prearc.place] & CAN_INC) > 0;
                if (!notInhibited || !enoughTokens) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Remove places that cannot increase nor decrease as well as unfireable transitions
        bool anyRemoved = false;
        for (uint32_t p = 0; p < parent->_places.size(); ++p) {
            if (!parent->_places[p].skip && placeInQuery[p] == 0 && _pflags[p] == 0) {
                skipPlace(p);
                anyRemoved = true;
            }
        }
        for (uint32_t t = 0; t < parent->_transitions.size(); ++t) {
            if (!parent->_transitions[t].skip && _tflags[t] == 0) {
                skipTransition(t);
                anyRemoved = true;
            }
        }
        if (anyRemoved) {
            _ruleM++;
            return true;
        }
        return false;
    }

    bool Reducer::ReducebyRuleEFMNOP(uint32_t* placeInQuery) {
        // Removes dead and redundant places, transitions, and arcs.
        // Using fixed-point iteration find an over-approximation of which places will never gain or lose tokens,
        // by considering initially enabled transitions, and transitions that may be fireable later due to knowledge
        // of which places can gain or lose tokens. We remove places that cannot gain or lose tokens, since they add
        // no behaviour, and we remove any transition that is not fireable (Rule M).
        // In the fixed-point iteration we also keep track of lower bounds induced by fireable transitions with
        // negative effect. We can then remove arcs that never disables the given transition (Rule N) and remove
        // transitions which are always inhibited (Rule O).
        // For transitions that cannot gain tokens, we can remove inhibitor arcs that never inhibits.
        // If a place only has transitions with positive effect and no inhibitor arcs, then it is removed too (Rule F).

        if (hasTimedout()) return false;
        bool continue_reductions = false;
        // Use two greatest bits of pflags to keep track of places that can increase or decrease their number of tokens.
        const uint8_t CAN_INC =  0b10000000u;
        const uint8_t CAN_DEC =  0b01000000u;
        _pflags.resize(parent->_places.size(), 0);
        std::fill(_pflags.begin(), _pflags.end(), 0);

        // Initialize lower bound as initial marking
        _lower.resize(parent->_places.size(), 0);
        std::fill(_lower.begin(), _lower.end(), 0);
        std::copy(parent->initialMarking.begin(), parent->initialMarking.end(), _lower.begin());

        // Use tflags to mark processed fireable transitions
        _tflags.resize(parent->_transitions.size(), 0);
        std::fill(_tflags.begin(), _tflags.end(), 0);

        // Queue of potentially fireable transitions to process
        std::queue<uint32_t> queue;

        auto processIncPlace = [&](uint32_t p) {
            if ((_pflags[p] & CAN_INC) == 0) {
                // Mark place as increasing
                _pflags[p] |= CAN_INC;
                Place place = parent->_places[p];
                for (uint32_t t : place.consumers) {
                    if (_tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processDecPlace = [&](uint32_t p, uint32_t low) {
            // Update lower bound
            _lower[p] = std::min(low, _lower[p]);
            if ((_pflags[p] & CAN_DEC) == 0) {
                // Mark place as decreasing
                _pflags[p] |= CAN_DEC;
                Place place = parent->_places[p];
                for (uint32_t t : place.consumers) {
                    if (_tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processEnabled = [&](uint32_t t) {
            _tflags[t] = 1;
            Transition& tran = parent->_transitions[t];
            // Find and process negative preset and positive postset
            uint32_t i = 0, j = 0;
            while (i < tran.pre.size() && j < tran.post.size())
            {
                if (tran.pre[i].place < tran.post[j].place) {
                    if (!tran.pre[i].inhib)
                        processDecPlace(tran.pre[i].place, 0);
                    i++;
                } else if (tran.pre[i].place > tran.post[j].place) {
                    processIncPlace(tran.post[j].place);
                    j++;
                } else {
                    if (tran.pre[i].inhib) {
                        processIncPlace(tran.post[j].place);
                    } else {
                        // There are both an in and an out arc to this place. Is the effect non-zero?
                        if (tran.pre[i].weight > tran.post[j].weight) {
                            processDecPlace(tran.pre[i].place, tran.post[j].weight);
                        } else if (tran.pre[i].weight < tran.post[j].weight) {
                            processIncPlace(tran.post[j].place);
                        }
                    }
                    i++; j++;
                }
            }
            for ( ; i < tran.pre.size(); i++) {
                if (!tran.pre[i].inhib)
                    processDecPlace(tran.pre[i].place, 0);
            }
            for ( ; j < tran.post.size(); j++) {
                processIncPlace(tran.post[j].place);
            }
        };

        // Process initially enabled transitions
        for (uint32_t t = 0; t < parent->_transitions.size(); ++t) {
            Transition& tran = parent->_transitions[t];
            if (tran.skip)
                continue;
            bool enabled = true;
            for (Arc& prearc : tran.pre) {
                if (prearc.inhib != (prearc.weight > parent->initialMarking[prearc.place])) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Now we find the fixed point of CAN_INC, CAN_DEC, and _tflags iteratively

        while (!queue.empty()) {
            if (hasTimedout()) return false;

            uint32_t t = queue.front();
            queue.pop();
            if (_tflags[t] == 1) continue;

            // Is t enabled?
            bool enabled = true;
            for (Arc prearc : parent->_transitions[t].pre) {
                bool notInhibited = !prearc.inhib || prearc.weight > parent->initialMarking[prearc.place] || (_pflags[prearc.place] & CAN_DEC) > 0;
                bool enoughTokens = prearc.inhib || prearc.weight <= parent->initialMarking[prearc.place] || (_pflags[prearc.place] & CAN_INC) > 0;
                if (!notInhibited || !enoughTokens) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        for (uint32_t t = 0; t < parent->_transitions.size(); ++t) {
            if (!parent->_transitions[t].skip && _tflags[t] == 0) {
                skipTransition(t);
                _ruleE++;
                continue_reductions = true;
            }
        }
        assert(consistent());
        for (uint32_t p = 0; p < parent->_places.size(); ++p) {
            Place& place = parent->_places[p];
            if(place.skip) continue;
            if (_pflags[p] == 0) {
                // Remove places that cannot increase nor decrease (Rule M)
                ++_ruleM;

                if(placeInQuery[p] == 0)
                {
                    skipPlace(p);
                    continue_reductions = true;
                }
                else
                {
                    for(auto t : place.consumers)
                    {
                        auto& trans = getTransition(t);
                        auto inArc = getInArc(p, trans);
                        trans.pre.erase(inArc);
                    }
                    for(auto t : place.producers)
                    {
                        auto& trans = getTransition(t);
                        auto arc = getOutArc(trans, p);
                        trans.post.erase(arc);
                    }
                    continue_reductions |= (place.consumers.size() + place.producers.size()) > 0;
                    place.producers.clear();
                    place.consumers.clear();
                }
                assert(consistent());
            }
            else
            {
                assert(consistent());
                bool all_ok = true;
                for(int64_t i = (int64_t)(place.consumers.size())-1; i >= 0; --i)
                {
                    auto t = place.consumers[i];
                    auto& trans = getTransition(t);
                    auto inArc = getInArc(p, trans);
                    if(inArc->inhib)
                    {
                        if(inArc->weight <= _lower[p])
                        {
                            // always disabled
                            skipTransition(t);
                            ++_ruleO;
                            continue_reductions = true;
                        }
                        else if((_pflags[p] & CAN_INC) == 0 && inArc->weight > parent->initialMarking[p])
                        {
                            // inhibitor is useless
                            trans.pre.erase(inArc);
                            place.consumers.erase(place.consumers.begin() + i);
                            ++_ruleP;
                            continue_reductions = true;
                        }
                        else all_ok = false;
                    }
                    else
                    {
                        if(inArc->weight > parent->initialMarking[p] && (_pflags[p] & CAN_INC) == 0)
                        {
                            skipTransition(t); // technically this should never happen I suppose.
                            ++_ruleE;
                            continue_reductions = true;
                            continue;
                        }
                        else if(inArc->weight > _lower[p]) // this is implied by above check also
                        {
                            // this arc may or may not be satisfied, nothing conclusive to say
                            all_ok = false;
                            continue;
                        }

                        // all other arcs are *permanently* satisfied, so we can
                        // compile them into their joint effect.
                        auto& trans = getTransition(t);
                        auto out = getOutArc(trans, p);
                        if(out != std::end(trans.post))
                        {
                            if(out->weight > inArc->weight)
                            {
                                out->weight -= inArc->weight;
                                trans.pre.erase(inArc);
                                place.consumers.erase(place.consumers.begin() + i);
                                ++_ruleN;
                                continue_reductions = true;
                            }
                            else if(out->weight == inArc->weight)
                            {
                                trans.pre.erase(inArc);
                                place.consumers.erase(place.consumers.begin() + i);
                                skipOutArc(t, p);
                                ++_ruleN;
                                continue_reductions = true;
                            }
                            else {
                                assert(false);
                                all_ok = false;
                            }
                        }
                        else
                        {
                            assert(false);
                            all_ok = false;
                        }
                    }
                }
                if(all_ok)
                {
                    // all consuming arcs of the place are always either satisfied
                    // or not satisfied (and removed above) so we can remove
                    // the place assuming that it is not observable.
                    if(placeInQuery[p] == 0)
                    {
                        skipPlace(p);
                        ++_ruleF;
                        continue_reductions = true;
                    }
                }
            }
        }
        assert(consistent());

        return continue_reductions;
    }

    bool Reducer::ReducebyRuleQ(uint32_t* placeInQuery)
    {
        // Fire initially enabled transitions if they are the single consumer of their preset
        if(reconstructTrace)
            return false;
        bool continueReductions = false;

        for (uint32_t t = 0; t < parent->numberOfTransitions(); ++t)
        {
            Transition& tran = parent->_transitions[t];

            if (tran.skip || tran.inhib || tran.pre.empty()) continue;

            // We take advantage of pre and post being sorted as well as the overloaded < operator to check:
            // - Preset and postset must be disjoint (to avoid infinite use)
            // - Preset and postset cannot inhibit or be in query
            // - Preset can only have this transition in postset
            // - How many times can we fire the transition
            uint32_t k = 0;
            bool ok = true;
            uint32_t i = 0, j = 0;
            while (i < tran.pre.size() || j < tran.post.size())
            {
                if (i < tran.pre.size() && (j == tran.post.size() || tran.pre[i] < tran.post[j]))
                {
                    const Arc& prearc = tran.pre[i];
                    uint32_t n = parent->initialMarking[prearc.place] / prearc.weight;
                    if (n == 0 ||
                        parent->_places[prearc.place].inhib ||
                        placeInQuery[prearc.place] > 0 ||
                        parent->_places[prearc.place].consumers.size() != 1)
                    {
                        ok = false;
                        break;
                    }
                    else
                    {
                        if (k == 0) k = n;
                        else k = std::min(k, n);
                    }
                    i++;
                }
                else if (j < tran.post.size() && (i == tran.pre.size() || tran.post[j] < tran.pre[i]))
                {
                    const Arc& postarc = tran.post[j];
                    if (parent->_places[postarc.place].inhib || placeInQuery[postarc.place] > 0)
                    {
                        ok = false;
                        break;
                    }
                    j++;
                }
                else
                {
                    ok = false;
                    break;
                }
            }

            if (!ok || k == 0) continue;

            // Update initial marking
            for (const Arc& prearc : tran.pre)
            {
                parent->initialMarking[prearc.place] -= prearc.weight * k;
            }
            for (const Arc& postarc : tran.post)
            {
                parent->initialMarking[postarc.place] += postarc.weight * k;
            }
            if(reconstructTrace)
            {
                for(size_t i = 0; i < k; ++i)
                    _initfire.emplace_back(getTransitionName(t));
            }

            _ruleQ++;
            continueReductions = true;
        }

        return continueReductions;
    }

    bool Reducer::ReducebyRuleR(uint32_t* placeInQuery, uint32_t explosion_limiter)
    {
        // Rule R performs post agglomeration on a single producer, merging its firing with all consumers
        if(reconstructTrace) // current reconstruction concept does not extend to handle ruleS. The trivial cases are already dealt with in ruleA and ruleB
            return false;
        bool continueReductions = false;

        for (uint32_t pid = 0; pid < parent->numberOfPlaces(); pid++)
        {
            if (hasTimedout())
                return false;

            const Place& place = parent->_places[pid];

            if (place.skip || place.inhib || placeInQuery[pid] > 0 || place.producers.empty() || place.consumers.empty())
                continue;

            // Check that prod and cons are disjoint
            const auto presize = place.producers.size();
            const auto postsize = place.consumers.size();
            const auto expl = presize*postsize;
            int64_t n_new_trans = 0;
            if(expl > std::max(explosion_limiter, (uint32_t)_skippedTransitions.size()))
                continue;
            bool ok = true;
            uint32_t i = 0, j = 0;
            while (i < presize && j < postsize)
            {
                if (place.producers[i] < place.consumers[j])
                    i++;
                else if (place.consumers[j] < place.producers[i])
                    j++;
                else
                {
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            // Now we analyze consumers further
            uint32_t maxConW = 0;
            for (auto con : place.consumers)
            {
                // Consumers may not be inhibited and only consume from pid.
                const Transition& tran = parent->_transitions[con];
                if (tran.inhib || tran.pre.size() != 1)
                {
                    ok = false;
                    break;
                }

                // Post-set of consumers may not inhibit or appear in query.
                for (const Arc& arc : tran.post)
                {
                    if (placeInQuery[arc.place] > 0 || parent->_places[arc.place].inhib)
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok) break;

                // Find the greatest weight between pid and consumers
                maxConW = std::max(maxConW, tran.pre[0].weight);
            }

            if (!ok) continue;

            // Find producers for which we can fuse its firing with a consumer
            bool removedAllProducers = true;
            auto prods_todo = place.producers;
            while (!prods_todo.empty())
            {
                auto prod_id = prods_todo.back();
                prods_todo.pop_back();

                if (hasTimedout())
                    return false;

                Transition prod = parent->_transitions[prod_id];
                auto prodArc = getOutArc(prod, pid);

                if (prodArc->weight < maxConW)
                {
                    removedAllProducers = false;
                    continue;
                }

                // TODO Build all full combinations in one go, instead of appending one consumer at a time
                // Combine producer with the consumers
                for (auto con_id : place.consumers)
                {
                    // Create new transition with effect of firing the producer and then the consumer
                    auto id = parent->_transitions.size();
                    if (!_skippedTransitions.empty())
                    {
                        id = _skippedTransitions.back();
                        _skippedTransitions.pop_back();
                    }
                    else
                    {
                        parent->_transitions.emplace_back();
                        parent->_transitionnames[newTransName()] = id;
                        parent->_transitionlocations.emplace_back(std::tuple<double, double>(0.0, 0.0));
                    }
                    ++n_new_trans;
                    Transition& newtran = parent->_transitions[id];
                    newtran.skip = false;
                    newtran.inhib = false;

                    // Arcs from consumer
                    Transition& cons = parent->_transitions[con_id];
                    for (const auto& arc : cons.post)
                    {
                        newtran.addPostArc(arc);
                    }
                    // Arcs from producer
                    for (const auto& arc : prod.pre)
                    {
                        newtran.addPreArc(arc);
                    }
                    for (const auto& arc : prod.post)
                    {
                        if (arc.place == pid)
                        {
                            Arc leftoverArc = arc; // Copy!
                            leftoverArc.weight -= cons.pre[0].weight;
                            if (leftoverArc.weight > 0) {
                                newtran.addPostArc(leftoverArc);
                                // The new transition is also a producer, so we will process it again
                                prods_todo.push_back(id);
                            }
                        }
                        else
                        {
                            newtran.addPostArc(arc);
                        }
                    }

                    for(const auto& arc : newtran.pre)
                        parent->_places[arc.place].addConsumer(id);
                    for(const auto& arc : newtran.post)
                        parent->_places[arc.place].addProducer(id);
                }

                skipTransition(prod_id);
                --n_new_trans;
                continueReductions = true;
                _ruleR++;
            }

            if (removedAllProducers && parent->initialMarking[pid] == 0)
            {
                auto consumers = place.consumers;
                for (auto cons_id : consumers)
                {
                    skipTransition(cons_id);
                    --n_new_trans;
                }

                skipPlace(pid);
            }

            consistent();
            if(n_new_trans > (int64_t)explosion_limiter) // we did something that expands the number of transitions significantly
                return continueReductions;
        }
        return continueReductions;
    }

    bool Reducer::ReducebyRuleS(uint32_t* placeInQuery, bool remove_consumers, bool remove_loops, bool allReach, uint32_t explosion_limiter) {
        if(reconstructTrace) // current reconstruction concept does not extend to handle ruleS. The trivial cases are already dealt with in ruleA and ruleB
            return false;
        bool continueReductions = false;
        bool atomic_viable = allReach && remove_loops;

        for (uint32_t pid = 0; pid < parent->numberOfPlaces(); pid++) {
            if (hasTimedout())
                return false;

            const Place &place = parent->_places[pid];

            // T8/S8--1, T7/S7--1
            if (place.skip || place.inhib || placeInQuery[pid] > 0 || place.producers.empty() ||
                place.consumers.empty())
                continue;

            int64_t n_added = 0;

            // Check that prod and cons are disjoint
            // T4/S4
            const auto presize = place.producers.size();
            const auto postsize = place.consumers.size();
            bool ok = true;
            uint32_t i = 0, j = 0;
            while (i < presize && j < postsize) {
                if (place.producers[i] < place.consumers[j])
                    i++;
                else if (place.consumers[j] < place.producers[i])
                    j++;
                else {
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            // S5
            std::vector<bool> todo (postsize, true);
            bool todoAllGood = true;
            // S10, S11
            std::vector<bool> kIsAlwaysOne (postsize, true);

            for (const auto& prod : place.producers){
                Transition& producer = getTransition(prod);
                // T8/S8--2, T6/S6
                if(producer.inhib || producer.post.size() != 1){
                    ok = false;
                    break;
                }

                uint32_t kw = getOutArc(producer, pid)->weight;
                for (uint32_t n = 0; n < place.consumers.size(); n++) {
                    uint32_t w = getInArc(pid, getTransition(place.consumers[n]))->weight;
                    if (atomic_viable){
                        // S3, S9
                        if (parent->initialMarking[pid] >= w || kw % w != 0) {
                            // Atomic is only valid for reachability without deadlock.
                            todo[n] = false;
                            todoAllGood = false;
                        } else if (kw != w) {
                            kIsAlwaysOne[n] = false;
                        }
                    // T3, T9
                    } else if (parent->initialMarking[pid] >= w || kw != w) {
                        ok = false;
                        break;
                    }
                }

                if (!ok) break;

                // Check if we have any qualifying consumers left
                if (!todoAllGood && std::lower_bound(todo.begin(), todo.end(), true) == todo.end()){
                    ok = false;
                    break;
                }

                for (const auto& prearc : producer.pre){
                    const auto& preplace = parent->_places[prearc.place];
                    // T8/S8--3, T7/S7--2
                    if (preplace.inhib || placeInQuery[prearc.place] > 0){
                        ok = false;
                        break;
                    } else if (!atomic_viable) {
                        // For reachability we can do free agglomeration which avoids this condition
                        // T5: Only transitions in place.producers are allowed in preplace.consumers.
                        // Reuses i and j that are done being used but still in scope.
                        i = 0;
                        j = 0;
                        while (i < preplace.consumers.size() && j < place.producers.size()) {
                            if (preplace.consumers[i] > place.producers[j])
                                j++;
                            else if (preplace.consumers[i] == place.producers[j]){
                                i++;
                                j++;
                            } else {
                                ok = false;
                                break;
                            }
                        }
                        if (i < preplace.consumers.size()){
                            // In case the while was exited by reaching the end of place.producers
                            ok = false;
                        }
                    }
                    if (!ok) break;
                }
                if (!ok) break;
            }

            if (!ok) continue;
            std::vector<uint32_t> originalConsumers = place.consumers;
            std::vector<uint32_t> originalProducers = place.producers;
            for (uint32_t n = 0; n < originalConsumers.size(); n++)
            {
                if (hasTimedout())
                    return false;
                if (!todo[n])
                    continue;

                ok = true;
                Transition &consumer = getTransition(originalConsumers[n]);
                // S10, S11
                if (atomic_viable && !kIsAlwaysOne[n]) {
                    // S11
                    if (consumer.pre.size() != 1){
                        continue;
                    }
                    // S10
                    for (const auto& conspost : consumer.post) {
                        if (!kIsAlwaysOne[n] && parent->_places[conspost.place].inhib){
                            ok = false;
                            break;
                        } else if (!remove_loops && placeInQuery[conspost.place] > 0){
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok) continue;
                // Update
                for (const auto& prod : originalProducers){
                    Transition &producer = getTransition(prod);

                    if (atomic_viable){
                        // Rule S updates

                        uint32_t k = 1, w = 1;
                        if (!kIsAlwaysOne[n]) {
                            w = getInArc(pid, consumer)->weight;
                            k = getOutArc(producer, pid)->weight / w;
                        }

                        // One transition for each number of firings of consumer possible after one firing of producer
                        for (uint32_t k_i = 1; k_i <= k; k_i++){
                            // Create new transition with effect of firing the producer, and then the consumer k_i times
                            auto id = parent->_transitions.size();
                            if (!_skippedTransitions.empty())
                            {
                                id = _skippedTransitions.back();
                                _skippedTransitions.pop_back();
                            }
                            else
                            {
                                parent->_transitions.emplace_back();
                                parent->_transitionnames[newTransName()] = id;
                                parent->_transitionlocations.emplace_back(std::tuple<double, double>(0.0, 0.0));
                            }
                            ++n_added;

                            // Re-fetch the transition pointers as it might be invalidated, I think that's the issue?
                            Transition &producerPrime = getTransition(prod);
                            Transition &consumerPrime = getTransition(originalConsumers[n]);
                            Transition& newtran = parent->_transitions[id];
                            newtran.skip = false;
                            newtran.inhib = false;

                            // Arcs from consumer
                            for (const auto& arc : consumerPrime.post) {
                                Arc newarc = arc;
                                newarc.weight = newarc.weight * k_i;
                                newtran.addPostArc(newarc);
                            }
                            for (const auto& arc : consumerPrime.pre){
                                if (arc.place != pid){
                                    Arc newarc = arc;
                                    newarc.weight = newarc.weight * k_i;
                                    newtran.addPreArc(arc);
                                    newtran.inhib |= arc.inhib;
                                }
                            }

                            for (const auto& arc : producerPrime.pre){
                                newtran.addPreArc(arc);
                            }

                            if (k_i != k){
                                Arc newarc = producerPrime.post[0];
                                newarc.weight = (k-k_i)*w;
                                newtran.addPostArc(newarc);
                            }

                            for(const auto& arc : newtran.pre){
                                parent->_places[arc.place].addConsumer(id);
                                parent->_places[arc.place].inhib |= arc.inhib;
                            }
                            for(const auto& arc : newtran.post)
                                parent->_places[arc.place].addProducer(id);
                        }
                    } else {
                        // Rule T updates
                        // Create new transition with effect of firing the producer, and then the consumer
                        auto id = parent->_transitions.size();
                        if (!_skippedTransitions.empty())
                        {
                            id = _skippedTransitions.back();
                            _skippedTransitions.pop_back();
                        }
                        else
                        {
                            parent->_transitions.emplace_back();
                            parent->_transitionnames[newTransName()] = id;
                            parent->_transitionlocations.emplace_back(std::tuple<double, double>(0.0, 0.0));
                        }
                        ++n_added;

                        // Re-fetch the transition pointers as it might be invalidated, I think that's the issue?
                        Transition &producerPrime = getTransition(prod);
                        Transition &consumerPrime = getTransition(originalConsumers[n]);
                        Transition& newtran = parent->_transitions[id];
                        newtran.skip = false;
                        newtran.inhib = false;

                        // Arcs from consumer
                        for (const Arc arc : consumerPrime.post) {
                            newtran.addPostArc(arc);
                        }
                        for (const Arc arc : consumerPrime.pre){
                            if (arc.place != pid){
                                newtran.addPreArc(arc);
                                newtran.inhib |= arc.inhib;
                            }
                        }

                        for (const Arc arc : producerPrime.pre){
                            newtran.addPreArc(arc);
                        }

                        for(const auto& arc : newtran.pre){
                            parent->_places[arc.place].addConsumer(id);
                            parent->_places[arc.place].inhib |= arc.inhib;
                        }

                        for(const auto& arc : newtran.post)
                            parent->_places[arc.place].addProducer(id);
                    }
                }
                skipTransition(originalConsumers[n]);
                --n_added;
                continueReductions = true;
                _ruleS++;
            }

            if (place.consumers.empty()) {
                // The producers of place will become purely consuming transitions when it is gone
                auto transitions = place.producers;
                for (uint32_t tran_id : transitions)
                {
                    skipTransition(tran_id);
                    --n_added;
                }
                skipPlace(pid);
            }
            consistent();
            if(n_added >= (int64_t)explosion_limiter)
                return continueReductions;
        }

        return continueReductions;
    }

    void Reducer::Reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace, int timeout, bool remove_loops,
            bool all_reach, bool all_ltl, bool contains_next, std::vector<uint32_t>& reduction) {
        this->_timeout = timeout;
        _timer = std::chrono::high_resolution_clock::now();
        assert(consistent());
        constexpr uint32_t explosion_limiter = 6;

        this->reconstructTrace = reconstructTrace;
        if (enablereduction == 2) { // for k-boundedness checking only rules A, D and H are applicable
            bool changed = true;
            while (changed && !hasTimedout()) {
                changed = false;
                if(!contains_next)
                {
                    while(ReducebyRuleA(context.getQueryPlaceCount())) changed = true;
                    while(ReducebyRuleD(context.getQueryPlaceCount(), all_reach, false)) changed = true;
                    while(ReducebyRuleH(context.getQueryPlaceCount(), all_ltl)) changed = true;
                }
            }
        }
        else if (enablereduction == 1) {
            bool changed = false;
            bool RQ = false;
            do
            {
restart:
                if(remove_loops && !contains_next)
                    while(ReducebyRuleI(context.getQueryPlaceCount(), all_reach)) changed = true;
                while(ReducebyRuleJ(context.getQueryPlaceCount())) changed = true;
                do{
                    do { // start by rules that do not move tokens
                        changed = false;
                        while(ReducebyRuleEFMNOP(context.getQueryPlaceCount())) changed = true;
                        while(ReducebyRuleC(context.getQueryPlaceCount())) changed = true;
                        if(remove_loops && !contains_next)
                            while(ReducebyRuleF(context.getQueryPlaceCount())) changed = true;
                        if(!contains_next)
                        {
                            while(ReducebyRuleG(context.getQueryPlaceCount(), remove_loops, all_reach)) changed = true;
                            while(ReducebyRuleD(context.getQueryPlaceCount(), all_reach, remove_loops && all_ltl)) changed = true;
                            //changed |= ReducebyRuleK(context.getQueryPlaceCount(), remove_consumers); //Rule disabled as correctness has not been proved. Experiments indicate that it is not correct for CTL.
                        }
                    } while(changed && !hasTimedout());
                    if(!contains_next)
                    { // then apply tokens moving rules
                        while(ReducebyRuleB(context.getQueryPlaceCount(), remove_loops, all_reach)) changed = true;
                        while(ReducebyRuleA(context.getQueryPlaceCount())) changed = true;
                    }
                } while(changed && !hasTimedout());
                while(ReducebyRuleL(context.getQueryPlaceCount())) changed = true;
                if(!contains_next && !changed)
                {
                    // Only try RuleH last. It can reduce applicability of other rules.
                    while (ReducebyRuleH(context.getQueryPlaceCount(), all_ltl)) changed = true;
                    while (ReducebyRuleS(context.getQueryPlaceCount(), all_reach, remove_loops, all_reach, explosion_limiter)) changed = true;
                    if(all_ltl && !changed) // ruleR is a last resort
                        changed = ReducebyRuleR(context.getQueryPlaceCount(), explosion_limiter);
                }

                if(!changed && !RQ)
                {
                    if(!contains_next)
                    {
                        RQ = ReducebyRuleQ(context.getQueryPlaceCount());
                        if(RQ)
                            goto restart;
                    }
                }
                RQ = false;
            } while(!hasTimedout() && changed);
        }
        else
        {
            const char* rnames = "ABCDEFGHIJKLMNOPQRS";
            for(int i = reduction.size() - 1; i >= 0; --i)
            {
                if (contains_next && (reduction[i] == 2 || reduction[i] == 4 || reduction[i] == 5 || reduction[i] == 8))
                {
                    std::cerr << "Skipping Rule" << rnames[reduction[i]] << " due to NEXT operator in proposition" << std::endl;
                    reduction.erase(reduction.begin() + i);
                }
                else if (!remove_loops && (reduction[i] == 5 || reduction[i] == 8 || reduction[i] == 12))
                {
                    std::cerr << "Skipping Rule" << rnames[reduction[i]] << " as proposition is loop sensitive" << std::endl;
                    reduction.erase(reduction.begin() + i);
                }
                else if (!(all_ltl || all_reach) && (reduction[i] == 17 || reduction[i] == 18)) {
                    std::cerr << "Skipping Rule" << rnames[reduction[i]] << " as proposition is not LTL" << std::endl;
                    reduction.erase(reduction.begin() + i);
                }
            }
            bool changed = true;

            while(changed && !hasTimedout())
            {
                changed = false;
                for(auto r : reduction)
                {
#ifndef NDEBUG
                    auto c = std::chrono::high_resolution_clock::now();
                    auto op = numberOfUnskippedPlaces();
                    auto ot = numberOfUnskippedTransitions();
#endif
                    switch(r)
                    {
                        case 0:
                            while(ReducebyRuleA(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 1:
                            while(ReducebyRuleB(context.getQueryPlaceCount(), remove_loops, all_reach)) changed = true;
                            break;
                        case 2:
                            while(ReducebyRuleC(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 3:
                            while(ReducebyRuleD(context.getQueryPlaceCount(), all_reach, remove_loops && all_ltl)) changed = true;
                            break;
                        case 4:
                            while(ReducebyRuleEP(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 5:
                            while(ReducebyRuleF(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 6:
                            while(ReducebyRuleG(context.getQueryPlaceCount(), remove_loops, all_reach)) changed = true;
                            break;
                        case 7:
                            while(ReducebyRuleH(context.getQueryPlaceCount(), all_ltl)) changed = true;
                            break;
                        case 8:
                            while(ReducebyRuleI(context.getQueryPlaceCount(), all_reach)) changed = true;
                            break;
                        case 9:
                            break;
                        case 10:
                            if (ReducebyRuleK(context.getQueryPlaceCount(), all_reach)) changed = true;
                            break;
                        case 11:
                            if (ReducebyRuleL(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 12:
                            if (ReducebyRuleM(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 13:
                        case 14:
                            if (ReducebyRuleFNO(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 15:
                            if (ReducebyRuleEP(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 16:
                            if (ReducebyRuleQ(context.getQueryPlaceCount())) changed = true;
                            break;
                        case 17:
                            while (ReducebyRuleR(context.getQueryPlaceCount(), explosion_limiter)) changed = true;
                            break;
                        case 18:
                            if (ReducebyRuleS(context.getQueryPlaceCount(), all_reach, remove_loops, all_reach, explosion_limiter)) changed = true;
                            break;
                    }
#ifndef NDEBUG
                    auto end = std::chrono::high_resolution_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - c);
                    std::cout << "SPEND " << diff.count()  << " ON " << rnames[r] << std::endl;
                    std::cout << "REM " << ((int)op - (int)numberOfUnskippedPlaces()) << " " << ((int)ot - (int)numberOfUnskippedTransitions()) << std::endl;
#endif
                    if(hasTimedout())
                        break;
                }
            }
        }

        return;
    }

    void Reducer::saveInitialNet() {
        // Called by PetriNetBuilder::saveInitialNet()
        _transitionsBeforeReduction.reserve(parent->_transitions.size());
        uint32_t i = 0;
        for (const auto& trans : parent->_transitions) {
            for (const auto& arc : trans.pre) {
                if (!arc.inhib)
                    _transitionsBeforeReduction[*getTransitionName(i)].emplace_back(getPlaceName(arc.place), arc.weight);
            }
            ++i;
        }
    }

    void Reducer::postFire(std::ostream& out, const std::string& transition) const
    {
        auto it = _postfire.find(transition);
        if (it != std::end(_postfire))
        {
            for (const auto& el : it->second)
            {
                out << "\t<transition id=\"" << *el << "\">\n";
                tokenConsumption(out, *el);
                out << "\t</transition>\n";
                postFire(out, *el);
            }
        }
    }

    void Reducer::initFire(std::ostream& out) const
    {
        for (const auto& init : _initfire)
        {
            out << "\t<transition id=\"" << *init << "\">\n";
            tokenConsumption(out, *init);
            out << "\t</transition>\n";
            postFire(out, *init);
        }
    }

    void Reducer::tokenConsumption(std::ostream& out, const std::string& transition) const
    {
        auto it = _transitionsBeforeReduction.find(transition);
        if (it != std::end(_transitionsBeforeReduction))
        {
            for (const auto& arc : it->second)
            {
                out << arc;
            }
        }
    }

} //PetriNet namespace
