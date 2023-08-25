/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef STATESET_H
#define STATESET_H
#include <ptrie/ptrie_stable.h>
#include <ptrie/ptrie_map.h>
#include <unordered_map>
#include <iostream>
#include "State.h"
#include "AlignedEncoder.h"
#include "utils/structures/binarywrapper.h"
#include "utils/errors.h"
#include "PetriEngine/PQL/Contexts.h"


namespace PetriEngine {
    namespace Structures {
        class StateSetInterface
        {
        public:
            StateSetInterface(const PetriNet& net, uint32_t kbound, int nplaces = -1) :
            _nplaces(nplaces == -1 ? net.numberOfPlaces() : nplaces),
            _kbound(kbound), _net(net)
            {
                _discovered = 0;
                _maxTokens = 0;
                _maxPlaceBound = std::vector<uint32_t>(net.numberOfPlaces(), 0);
            }

            virtual ~StateSetInterface() {}

            const PetriNet& net() { return _net;}

            uint32_t maxTokens() const { return _maxTokens; }

            virtual size_t size() const = 0;

            size_t discovered() const {
                return _discovered;
            }

            virtual std::pair<size_t, size_t> getHistory(size_t markingid) = 0;

            const std::vector<MarkVal>& maxPlaceBound() const {
                return _maxPlaceBound;
            }


        protected:
            size_t _discovered;
            size_t _nplaces;
            uint32_t _kbound;
            uint32_t _maxTokens;
            std::vector<uint32_t> _maxPlaceBound;
            const PetriNet& _net;
        };

        /**
         * This class defines the state set for a random walk
         * It is used by the reachability search to store the states during one walk.
         */
        class RandomWalkStateSet : public StateSetInterface
        {
        public:
            RandomWalkStateSet(const PetriNet& net, uint32_t kbound, const PQL::Condition *query,
                               const std::vector<MarkVal> &initPotencies, size_t seed, int nplaces = -1)
                : StateSetInterface(net, kbound, nplaces), _seed(seed)
            {
                srand(_seed);
                _discovered = 1;
                _initialMarking = std::make_unique<MarkVal[]>(_nplaces);
                setMarking(net.makeInitialMarking(), _initialMarking.get());
                _nextMarking = std::make_unique<MarkVal[]>(_nplaces);
                _nextMarking[0] = std::numeric_limits<MarkVal>::max();

                if (initPotencies.empty()) {
                    _initializePotencies(_net.numberOfTransitions(), _initPotencyConstant);
                } else {
                    _initializePotencies(initPotencies);
                }
                PQL::DistanceContext context(&_net, _initialMarking.get());
                _initialDistance = query->distance(context);
            }

            ~RandomWalkStateSet() {}

            void setMarking(const MarkVal* source, MarkVal* target) {
                std::copy(source, source + _nplaces, target);
            }

            /**
             * Computes the candidate marking for the next step of the random walk.
             * Updates the potency of the given transition according to the distance of the candidate marking.
             * Updates the _nextMarking using a reservoir sampling algorithm.
             * @param candidate the candidate marking
             * @param query the query to check
             * @param t the transition
            */
            void computeCandidate(const MarkVal* candidate, const PQL::Condition *query, uint32_t t) {
                ++_discovered;
                PQL::DistanceContext context(&_net, candidate);

                uint32_t sumMarking = _sumMarking(candidate);
                if (_maxTokens < sumMarking) {
                    _maxTokens = sumMarking;
                }
                if (_kbound != 0 && sumMarking > _kbound)
                    return;

                // Update the max token bound for each place in the net (only for newly discovered markings)
                for (uint32_t i = 0; i < _net.numberOfPlaces(); i++)
                {
                    _maxPlaceBound[i] = std::max<MarkVal>(candidate[i], _maxPlaceBound[i]);
                }

                // Update the potency of the transition
                uint32_t dist = query->distance(context);
                if (dist < _currentStepDistance) {
                    _potencies[t] += _currentStepDistance - dist;
                } else {
                    if (_potencies[t] - 1 >= dist - _currentStepDistance)
                        _potencies[t] -= dist - _currentStepDistance;
                    else
                        _potencies[t] = 1;
                }

                // Weighted random sampling algorithm
                _totalWeight += _potencies[t];
                double r = (double)rand() / RAND_MAX;
                double threshold = _potencies[t] / (double)_totalWeight;
                if (r <= threshold) {
                   setMarking(candidate, _nextMarking.get());
                    _nextStepDistance = dist;
                }
            }

            /**
             * Resets the state set for a new random walk.
             * _nextMarking is set to the initial marking.
             * The _currentStepDistance is set to the distance of the initial marking.
            */
            void newWalk() {
                setMarking(_initialMarking.get(), _nextMarking.get());
                _currentStepDistance = _initialDistance;
            }

            /**
             * Prepare the next step of the random walk.
             * currentStepMarking is set to the _nextMarking.
             * _nextMarking is set to nullptr. _totalWeight is set to 0.
             * @return true if the walk can continue, false otherwise.
            */
            bool nextStep(MarkVal* currentStepMarking) {
                if (_nextMarking[0] == std::numeric_limits<MarkVal>::max()) {
                    return false;
                }
                setMarking(_nextMarking.get(), currentStepMarking);
                _currentStepDistance = _nextStepDistance;
                _totalWeight = 0;
                _nextMarking[0] = std::numeric_limits<MarkVal>::max();
                return true;
            }

            virtual size_t size() const override {
                // _discovered is used here but not sure it is the right value
                return discovered();
            }

            virtual std::pair<size_t, size_t> getHistory(size_t markingid) override {
                assert(false);
                return std::make_pair(0,0);
            }

        private:
            std::unique_ptr<MarkVal[]> _initialMarking;
            // The best candidate so far to be the next marking
            std::unique_ptr<MarkVal[]> _nextMarking;

            std::vector<uint32_t> _potencies;
            const static uint32_t _initPotencyConstant = 1;
            const static uint32_t _initPotencyMultiplier = 100;

            // Useful to update the potencies
            uint32_t _initialDistance;
            uint32_t _currentStepDistance;
            uint32_t _nextStepDistance;
            uint32_t _totalWeight;

            size_t _seed;

            void _initializePotencies(size_t nTransitions, uint32_t initValue) {
                _potencies = std::vector<uint32_t>(nTransitions, initValue);
            }

            void _initializePotencies(const std::vector<MarkVal> &initPotencies) {
                assert(initPotencies.size() == _net.numberOfTransitions());
                _potencies.reserve(initPotencies.size());
                uint32_t maxiInitPotencies = *std::max_element(initPotencies.begin(), initPotencies.end());
                for (auto potency : initPotencies) {
                    _potencies.push_back(potency * _initPotencyMultiplier / maxiInitPotencies + _initPotencyConstant);
                }
            }

            uint32_t _sumMarking(const MarkVal* marking) {
                uint32_t sum = 0;
                for (size_t i = 0; i < _nplaces; ++i) {
                    sum += marking[i];
                }
                return sum;
            }
        };

        class EncodingStateSetInterface : public StateSetInterface
        {
        public:
            EncodingStateSetInterface(const PetriNet& net, uint32_t kbound, int nplaces = -1) :
            StateSetInterface(net, kbound, nplaces), _encoder(_nplaces, kbound)
            {
                _sp = binarywrapper_t(sizeof(uint32_t) * _nplaces * 8);
            }

            virtual ~EncodingStateSetInterface()
            {
                _sp.release();
            }

            virtual std::pair<bool, size_t> add(const State* state) { return add(*state); };

            virtual std::pair<bool, size_t> add(const State& state) = 0;

            virtual void decode(State* state, size_t id) { decode(*state, id); }

            virtual void decode(State& state, size_t id) = 0;

            virtual std::pair<bool, size_t> lookup(State* state) { return lookup(*state); };

            virtual std::pair<bool, size_t> lookup(State &state) = 0;

            virtual void setHistory(size_t id, size_t transition) = 0;

        protected:
            AlignedEncoder _encoder;
            binarywrapper_t _sp;
#ifdef DEBUG
            std::vector<uint32_t*> _dbg;
#endif
            template<typename T>
            void _decode(State& state, size_t id, T& _trie)
            {
                    _trie.unpack(id, _encoder.scratchpad().raw());
                    _encoder.decode(state.marking(), _encoder.scratchpad().raw());

#ifdef DEBUG
                    assert(memcmp(state.marking(), _dbg[id], sizeof(uint32_t)*_net.numberOfPlaces()) == 0);
#endif
            }

            template<typename T>
            std::pair<bool, size_t> _add(const State& state, T& _trie) {
                _discovered++;

#ifdef DEBUG
                if(_discovered % 1000000 == 0) std::cout << "Found number " << _discovered << std::endl;
#endif

                MarkVal sum = 0;
                bool allsame = true;
                uint32_t val = 0;
                uint32_t active = 0;
                uint32_t last = 0;
                markingStats(state.marking(), sum, allsame, val, active, last);

                if (_maxTokens < sum)
                    _maxTokens = sum;

                //Check that we're within k-bound
                if (_kbound != 0 && sum > _kbound)
                    return std::pair<bool, size_t>(false, std::numeric_limits<size_t>::max());

                unsigned char type = _encoder.getType(sum, active, allsame, val);


                size_t length = _encoder.encode(state.marking(), type);
                if(length*8 >= std::numeric_limits<uint16_t>::max())
                {
                    throw base_error("Marking could not be encoded into less than 2^16 bytes, current limit of PTries");
                }
                binarywrapper_t w = binarywrapper_t(_encoder.scratchpad().raw(), length*8);
                auto tit = _trie.insert(w.raw(), w.size());


                if(!tit.first)
                {
                    return std::pair<bool, size_t>(false, tit.second);
                }

#ifdef DEBUG
                _dbg.push_back(new uint32_t[_net.numberOfPlaces()]);
                memcpy(_dbg.back(), state.marking(), _net.numberOfPlaces()*sizeof(uint32_t));
                decode(state, _trie.size() - 1);
#endif

                // update the max token bound for each place in the net (only for newly discovered markings)
                for (uint32_t i = 0; i < _net.numberOfPlaces(); i++)
                {
                    _maxPlaceBound[i] = std::max<MarkVal>( state.marking()[i],
                                                            _maxPlaceBound[i]);
                }

#ifdef DEBUG
                if(_trie.size() % 100000 == 0) std::cout << "Inserted " << _trie.size() << std::endl;
#endif
                return std::pair<bool, size_t>(true, tit.second);
            }

            template <typename T>
            std::pair<bool, size_t> _lookup(const State& state, T& _trie) {
                MarkVal sum = 0;
                bool allsame = true;
                uint32_t val = 0;
                uint32_t active = 0;
                uint32_t last = 0;
                markingStats(state.marking(), sum, allsame, val, active, last);

                unsigned char type = _encoder.getType(sum, active, allsame, val);

                size_t length = _encoder.encode(state.marking(), type);
                binarywrapper_t w = binarywrapper_t(_encoder.scratchpad().raw(), length*8);
                auto tit = _trie.exists(w.raw(), w.size());

                if (tit.first) {
                    return tit;
                }
                else return std::make_pair(false, std::numeric_limits<size_t>::max());
            }

            void markingStats(const uint32_t* marking, MarkVal& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last)
            {
                uint32_t cnt = 0;

                for (uint32_t i = 0; i < _nplaces; i++)
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
        };

#define STATESET_BUCKETS 1000000
        class StateSet : public EncodingStateSetInterface {
        private:
            using wrapper_t = ptrie::binarywrapper_t;
            using ptrie_t = ptrie::set_stable<ptrie::uchar,size_t,17,128,4>;

        public:
            using EncodingStateSetInterface::EncodingStateSetInterface;

            virtual std::pair<bool, size_t> add(const State& state) override
            {
                return _add(state, _trie);
            }

            virtual void decode(State& state, size_t id) override
            {
                _decode(state, id, _trie);
            }

            virtual std::pair<bool, size_t> lookup(State& state) override
            {
                return _lookup(state, _trie);
            }

            virtual void setHistory(size_t id, size_t transition) override {}

            virtual std::pair<size_t, size_t> getHistory(size_t markingid) override
            {
                assert(false);
                return std::make_pair(0,0);
            }

            virtual size_t size() const override {
                return _trie.size();
            }

        private:
            ptrie_t _trie;
        };

        template<typename T>
        class AnnotatedStateSet : public EncodingStateSetInterface {
        private:
            using ptrie_t = ptrie::map<unsigned char, T>;

        public:
            using EncodingStateSetInterface::EncodingStateSetInterface;

            virtual std::pair<bool, size_t> add(const State& state) override
            {
                return _add(state, _trie);
            }

            virtual void decode(State& state, size_t id) override
            {
                _decode(state, id, _trie);
            }

            T& get_data(size_t markingid){
                return _trie.get_data(markingid);
            }

            virtual std::pair<bool, size_t> lookup(State& state) override
            {
                return _lookup(state, _trie);
            }

            virtual void setHistory(size_t id, size_t transition) override {}

            virtual std::pair<size_t, size_t> getHistory(size_t markingid) override
            {
                assert(false);
                return std::make_pair(0,0);
            }

            virtual size_t size() const override {
                return _trie.size();
            }

        protected:
            ptrie_t _trie;
        };


        struct traceable_t
        {
            ptrie::uint parent;
            ptrie::uint transition;
        };

        class TracableStateSet : public AnnotatedStateSet<traceable_t>
        {

        public:
            using AnnotatedStateSet<traceable_t>::AnnotatedStateSet;

            virtual void decode(State& state, size_t id) override
            {
                _parent = id;
                AnnotatedStateSet<traceable_t>::decode(state, id);
            }

            virtual void setHistory(size_t id, size_t transition) override
            {
                traceable_t& t = get_data(id);
                t.parent = _parent;
                t.transition = transition;
            }

            virtual std::pair<size_t, size_t> getHistory(size_t markingid) override
            {
                traceable_t& t = get_data(markingid);
                return std::pair<size_t, size_t>(t.parent, t.transition);
            }

        private:
            size_t _parent = 0;
        };

        
    }
}


#endif // STATESET_H
