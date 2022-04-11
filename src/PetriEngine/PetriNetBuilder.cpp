/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <algorithm>

#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reducer.h"
#include <PetriEngine/PQL/PredicateCheckers.h>
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Analyze.h"


namespace PetriEngine {

    PetriNetBuilder::PetriNetBuilder(shared_string_set& string_set) : AbstractPetriNetBuilder(),
    reducer(this), _string_set(string_set) {
    }
    PetriNetBuilder::PetriNetBuilder(const PetriNetBuilder& other)
    : _placenames(other._placenames), _transitionnames(other._transitionnames),
       _placelocations(other._placelocations), _transitionlocations(other._transitionlocations),
       _transitions(other._transitions), _places(other._places),
       initialMarking(other.initialMarking), reducer(this), _string_set(other._string_set)
    {

    }

    PetriNetBuilder::PetriNetBuilder(PetriNetBuilder&& other)
    : _placenames(std::move(other._placenames)), _transitionnames(std::move(other._transitionnames)),
       _placelocations(std::move(other._placelocations)), _transitionlocations(std::move(other._transitionlocations)),
       _transitions(std::move(other._transitions)), _places(std::move(other._places)),
       initialMarking(std::move(other.initialMarking)), reducer(this), _string_set(other._string_set) {}

    void PetriNetBuilder::addPlace(const std::string &name, uint32_t tokens, double x, double y)
    {
        auto spn = std::make_shared<const_string>(name);
        return addPlace(spn, tokens, x, y);
    }

    void PetriNetBuilder::addPlace(const shared_const_string &_name, uint32_t tokens, double x, double y) {
        auto name = *_string_set.insert(_name).first;
        size_t size = _placenames.size();
        auto [it, inserted] = _placenames.insert(std::make_pair(name, size));
        if(inserted)
        {
            _places.emplace_back();
            _placelocations.push_back(std::tuple<double, double>(x,y));
        }

        if(initialMarking.size() <= it->second)
            initialMarking.resize(initialMarking.size() + 1, 0);
        initialMarking[it->second] = tokens;
    }

    void PetriNetBuilder::addTransition(const std::string &name,
            int32_t player, double x, double y) {
        auto stn = std::make_shared<const_string>(name);
        return addTransition(stn, player, x, y);
    }

    void PetriNetBuilder::addTransition(const shared_const_string &_name,
            int32_t player, double x, double y) {
        auto name = *_string_set.insert(_name).first;
        size_t size = _transitionnames.size();
        auto [it, inserted] = _transitionnames.insert(std::make_pair(name, size));
        if(inserted)
        {
            _transitions.emplace_back();
            _transitions.back()._player = player;
            _transitionlocations.push_back(std::tuple<double, double>(x,y));
        }
    }

    void PetriNetBuilder::addInputArc(const std::string &place, const std::string &transition, bool inhibitor, uint32_t weight)
    {
        auto spn = std::make_shared<const_string>(place);
        auto stn = std::make_shared<const_string>(transition);
        return addInputArc(spn, stn, inhibitor, weight);
    }

    void PetriNetBuilder::addInputArc(const shared_const_string &place, const shared_const_string &transition, bool inhibitor, uint32_t weight) {
        if(_transitionnames.count(transition) == 0)
        {
            throw base_error("Could not find ", transition);
        }
        if(_placenames.count(place) == 0)
        {
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        Arc arc;
        arc.place = p;
        arc.weight = weight;
        arc.skip = false;
        arc.inhib = inhibitor;
        assert(t < _transitions.size());
        assert(p < _places.size());
        _transitions[t].pre.push_back(arc);
        _transitions[t].inhib |= inhibitor;
        assert(std::find(_places[p].consumers.begin(), _places[p].consumers.end(), t) == std::end(_places[p].consumers));
        _places[p].consumers.push_back(t);
        _places[p].inhib |= inhibitor;
    }

    void PetriNetBuilder::addOutputArc(const std::string &transition, const std::string &place, uint32_t weight) {
        auto spn = std::make_shared<const_string>(place);
        auto stn = std::make_shared<const_string>(transition);
        return addOutputArc(stn, spn, weight);
    }

    void PetriNetBuilder::addOutputArc(const shared_const_string &transition, const shared_const_string &place, uint32_t weight) {
        if(_transitionnames.count(transition) == 0)
        {
            throw base_error("Could not find ", transition);
        }
        if(_placenames.count(place) == 0)
        {
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];
        assert(t < _transitions.size());
        assert(p < _places.size());

        Arc arc;
        arc.place = p;
        arc.weight = weight;
        arc.skip = false;
        assert(std::find(_places[p].producers.begin(), _places[p].producers.end(), t) == std::end(_places[p].producers));

        _transitions[t].post.push_back(arc);
        _places[p].producers.push_back(t);
    }

    uint32_t PetriNetBuilder::nextPlaceId(std::vector<uint32_t>& counts, std::vector<uint32_t>& pcounts, std::vector<uint32_t>& ids, bool reorder)
    {
        uint32_t cand = std::numeric_limits<uint32_t>::max();
        uint32_t cnt =  std::numeric_limits<uint32_t>::max();
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            uint32_t nnum = (pcounts[i] == 0 ? 0 : (counts[0] == 0 ? 0 : std::max(counts[i], pcounts[i])));
            if( ids[i] == std::numeric_limits<uint32_t>::max() &&
                nnum < cnt &&
                !_places[i].skip)
            {
                if(!reorder) return i;
                cand = i;
                cnt = nnum;
            }
        }
        return cand;
    }

    PetriNet* PetriNetBuilder::makePetriNet(bool reorder) {

        /*
         * The basic idea is to construct three arrays, the first array,
         * _invariants points to "arcs" - they are triplets (weight, place, inhibitor)
         * _transitions are pairs, (input, output) are indexes in the _invariants array
         * _placeToPtrs is an indirection going from a place-index to the FIRST transition
         *              with a non-inhibitor arc consuming from the given place.
         *
         * For all the indexes and indirections, notice that we only track the
         * beginning. We can naturally use the "next" value as the end. eg. the
         * inputs of a transition are between "input" and "output". The outputs
         * are between "output" and the "input" of the next transition.
         *
         * This allows us to quickly skip a lot of checks when generating successors
         * Beware that currently "orphans" and "inhibitor orphans" are special-cases
         * and currently handled as "consuming" from place id=0.
         *
         * If anybody wants to spend time on it, this is the first step towards
         * a decision-tree like construction, possibly improving successor generation.
         */

        uint32_t nplaces = _places.size() - reducer.RemovedPlaces();
        uint32_t ntrans = _transitions.size() - reducer.RemovedTransitions();

        std::vector<uint32_t> place_cons_count = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> place_prod_count = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> place_idmap = std::vector<uint32_t>(_places.size());
        std::vector<uint32_t> trans_idmap = std::vector<uint32_t>(_transitions.size());



        uint32_t invariants = 0;

        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            place_idmap[i] = std::numeric_limits<uint32_t>::max();
            if(!_places[i].skip)
            {
                place_cons_count[i] = _places[i].consumers.size();
                place_prod_count[i] = _places[i].producers.size();
#ifndef NDEBUG
                size_t cons = 0;
                size_t prod = 0;
                for(auto& t : _transitions)
                {
                    size_t pr = 0;
                    size_t po = 0;
                    for(auto& a : t.pre)
                        pr += (i == a.place ? 1 : 0);
                    for(auto& a : t.post)
                        po += (i == a.place ? 1 : 0);
                    assert(pr <= 1);
                    assert(po <= 1);
                    cons += pr;
                    prod += po;
                }
                assert(cons == place_cons_count[i]);
                assert(prod == place_prod_count[i]);
#endif
                invariants += _places[i].consumers.size() + _places[i].producers.size();
            }
        }

#ifndef NDEBUG
        std::vector<size_t> plpcount(_places.size(), 0);
        std::vector<size_t> plccount(_places.size(), 0);
#endif

        for(uint32_t i = 0; i < _transitions.size(); ++i)
        {
            trans_idmap[i] = std::numeric_limits<uint32_t>::max();
#ifndef NDEBUG
            for(auto& a : _transitions[i].pre)
                plccount[a.place] += 1;
            for(auto& a : _transitions[i].post)
                plpcount[a.place] += 1;
#endif
        }
#ifndef NDEBUG
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            assert(place_cons_count[i] == plccount[i]);
            assert(place_prod_count[i] == plpcount[i]);
        }
#endif

        PetriNet* net = new PetriNet(ntrans, invariants, nplaces);

        uint32_t next = nextPlaceId(place_cons_count, place_prod_count, place_idmap, reorder);
        uint32_t free = 0;
        uint32_t freeinv = 0;
        uint32_t freetrans = 0;

        // first handle orphans
        if(place_idmap.size() > next) place_idmap[next] = free;
        net->_placeToPtrs[free] = freetrans;
        for(size_t t = 0; t < _transitions.size(); ++t)
        {
            Transition& trans = _transitions[t];
            if (std::all_of(trans.pre.begin(), trans.pre.end(), [](Arc& a){return a.inhib;}))
            {
                // ALL have to be inhibitor, if any. Otherwise not orphan

                if(trans.skip) continue;
                net->_transitions[freetrans].inputs = freeinv;

                // add inhibitors
                for(const auto& pre : trans.pre)
                {
                    Invariant& iv = net->_invariants[freeinv];
                    iv.place = pre.place;
                    iv.tokens = pre.weight;
                    iv.inhibitor = pre.inhib;
                    assert(pre.inhib);
                    assert(place_cons_count[pre.place] > 0);
                    --place_cons_count[pre.place];
                    ++freeinv;
                }

                net->_transitions[freetrans].outputs = freeinv;

                for(const auto& post : trans.post)
                {
                    assert(freeinv < net->_ninvariants);
                    net->_invariants[freeinv].place = post.place;
                    net->_invariants[freeinv].tokens = post.weight;
                    ++freeinv;
                }

                trans_idmap[t] = freetrans;

                ++freetrans;
            }
        }

        bool first = true;
        while(next != std::numeric_limits<uint32_t>::max())
        {
            if(first) // already set for first iteration to handle orphans
            {
                first = false;
            }
            else
            {
                place_idmap[next] = free;
                net->_placeToPtrs[free] = freetrans;
            }

            for(auto t : _places[next].consumers)
            {
                Transition& trans = _transitions[t];
                if(trans.skip) continue;

                net->_controllable[freetrans] = trans._player == 0;
                net->_transitions[freetrans].inputs = freeinv;

                // check first, we are going to change state later, but we can
                // break here, so no statechange inside loop!
                bool ok = true;
                bool all_inhib = true;
                uint32_t cnt = 0;
                for(const Arc& pre : trans.pre)
                {
                    all_inhib &= pre.inhib;

                    // if transition belongs to previous place
                    if(     (!pre.inhib && place_idmap[pre.place] < free) ||
                            freeinv + cnt >= net->_ninvariants)
                    {
                        ok = false;
                        break;
                    }

                    // or arc from place is an inhibitor
                    if(pre.place == next &&  pre.inhib)
                    {
                        ok = false;
                        break;
                    }
                    ++cnt;
                }

                // skip for now, either T-a->P is inhibitor, or was allready added for other P'
                // or all a's are inhibitors.
                if(!ok || all_inhib) continue;

                trans_idmap[t] = freeinv;

                // everything is good, change state!.
                for(const auto& pre : trans.pre)
                {
                    Invariant& iv = net->_invariants[freeinv];
                    iv.place = pre.place;
                    iv.tokens = pre.weight;
                    iv.inhibitor = pre.inhib;
                    ++freeinv;
                    assert(place_cons_count[pre.place] > 0);
                    --place_cons_count[pre.place];
                }

                net->_transitions[freetrans].outputs = freeinv;
                for(const auto& post : trans.post)
                {
                    assert(freeinv < net->_ninvariants);
                    auto& post_inv = net->_invariants[freeinv];
                    post_inv.place = post.place;
                    post_inv.tokens = post.weight;
                    --place_prod_count[post.place];
                    ++freeinv;
                }

                trans_idmap[t] = freetrans;

                ++freetrans;
                assert(freeinv <= invariants);
            }
            ++free;
            next = nextPlaceId(place_cons_count, place_prod_count, place_idmap, reorder);
        }


        // Reindex for great justice!
        for(uint32_t i = 0; i < freeinv; i++)
        {
            net->_invariants[i].place = place_idmap[net->_invariants[i].place];
            assert(net->_invariants[i].place < nplaces);
            assert(net->_invariants[i].tokens > 0);
        }

//        std::cout << "init" << std::endl;
        for(uint32_t i = 0; i < _places.size(); ++i)
        {
            if(place_idmap[i] != std::numeric_limits<uint32_t>::max())
            {
                net->_initialMarking[place_idmap[i]] = initialMarking[i];
//                std::cout << place_idmap[i] << " : " << initialMarking[i] << std::endl;
            }
        }

        net->_placelocations = _placelocations;
        net->_transitionlocations = _transitionlocations;

        // reindex place-names
        net->_placenames.resize(_placenames.size());
        int rindex = _placenames.size() - 1;
        for(auto& i : _placenames)
        {
            auto& placelocation = _placelocations[i.second];
            i.second = place_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_placenames[i.second] = i.first;
                assert(_placenames[net->_placenames[i.second]] == i.second);
                net->_placelocations[i.second] = placelocation;
            }
            else
            {
                net->_placenames[rindex] = i.first;
                net->_placelocations[rindex] = placelocation;
                --rindex;
            }
        }
        net->_transitionnames.resize(_transitionnames.size());
        int trindex = _transitionnames.size() - 1;
        for(auto& i : _transitionnames)
        {
            auto& transitionlocation = _transitionlocations[i.second];
            i.second = trans_idmap[i.second];
            if(i.second != std::numeric_limits<uint32_t>::max())
            {
                net->_transitionnames[i.second] = i.first;
                net->_transitionlocations[i.second] = transitionlocation;
            }
            else
            {
                net->_transitionnames[trindex] = i.first;
                net->_transitionlocations[trindex] = transitionlocation;
                --trindex;
            }
        }
        net->sort();

        for(size_t t = 0; t < net->numberOfTransitions(); ++t)
        {
            {
                auto tiv = std::make_pair(&net->_invariants[net->_transitions[t].inputs], &net->_invariants[net->_transitions[t].outputs]);
                for(; tiv.first != tiv.second; ++tiv.first)
                {
                    tiv.first->direction = tiv.first->inhibitor ? 0 : -1;
                    bool found = false;
                    auto tov = std::make_pair(&net->_invariants[net->_transitions[t].outputs], &net->_invariants[net->_transitions[t + 1].inputs]);
                    for(; tov.first != tov.second; ++tov.first)
                    {
                        if(tov.first->place == tiv.first->place)
                        {
                            found = true;
                            if(tiv.first->inhibitor)                        tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens < tov.first->tokens)  tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens == tov.first->tokens) tiv.first->direction = tov.first->direction = 0;
                            else if(tiv.first->tokens > tov.first->tokens)  tiv.first->direction = tov.first->direction = -1;
                            break;
                        }
                    }
                    if(!found) assert(tiv.first->direction < 0 || tiv.first->inhibitor);
                }
            }
            {
                auto tiv = std::make_pair(&net->_invariants[net->_transitions[t].outputs], &net->_invariants[net->_transitions[t + 1].inputs]);
                for(; tiv.first != tiv.second; ++tiv.first)
                {
                    tiv.first->direction = 1;
                    bool found = false;
                    auto tov = std::make_pair(&net->_invariants[net->_transitions[t].inputs], &net->_invariants[net->_transitions[t].outputs]);
                    for(; tov.first != tov.second; ++tov.first)
                    {
                        found = true;
                        if(tov.first->place == tiv.first->place)
                        {
                            if     (tov.first->inhibitor)                   tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens > tov.first->tokens)  tiv.first->direction = tov.first->direction = 1;
                            else if(tiv.first->tokens == tov.first->tokens) tiv.first->direction = tov.first->direction = 0;
                            else if(tiv.first->tokens < tov.first->tokens)  tiv.first->direction = tov.first->direction = -1;
                            break;
                        }
                    }
                    if(!found) assert(tiv.first->direction > 0);
                }
            }
        }
        return net;
    }

    void PetriNetBuilder::sort()
    {
        for(Place& p : _places)
        {
            std::sort(p.consumers.begin(), p.consumers.end());
            std::sort(p.producers.begin(), p.producers.end());
        }

        for(Transition& t : _transitions)
        {
            std::sort(t.pre.begin(), t.pre.end());
            std::sort(t.post.begin(), t.post.end());
        }
    }

    void PetriNetBuilder::reduce(   std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                    std::vector<Reachability::ResultPrinter::Result>& results,
                                    int reductiontype, bool reconstructTrace, const PetriNet* net, int timeout, std::vector<uint32_t>& reductions)
    {
        QueryPlaceAnalysisContext placecontext(getPlaceNames(), getTransitionNames(), net);
        bool all_reach = true;
        bool remove_loops = true;
        bool contains_next = false;
        for(uint32_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == Reachability::ResultPrinter::Synthesis)
            {
                throw base_error("Reductions not supported due to 'control' predicate in query.");
                return; // we disable mode reductions if there is a synthesis query present.
            }
            if(results[i] == Reachability::ResultPrinter::Unknown ||
               results[i] == Reachability::ResultPrinter::CTL ||
               results[i] == Reachability::ResultPrinter::LTL)
            {
                PetriEngine::PQL::analyze(queries[i], placecontext);
                all_reach &= (results[i] != Reachability::ResultPrinter::CTL && results[i] != Reachability::ResultPrinter::LTL);
                remove_loops &= !PetriEngine::PQL::isLoopSensitive(queries[i]);
                // There is a deadlock somewhere, if it is not alone, we cannot reduce.
                // this has similar problems as nested next.
                contains_next |= PetriEngine::PQL::containsNext(queries[i]) || PetriEngine::PQL::hasNestedDeadlock(queries[i]);
            }
        }
        reducer.Reduce(placecontext, reductiontype, reconstructTrace, timeout, remove_loops, all_reach, contains_next, reductions);
    }


} // PetriEngine
