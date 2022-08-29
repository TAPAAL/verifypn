/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   CompoundStateSet.h
 * Author: Peter G. Jensen
 *
 * Created on 22 March 2022, 13.55
 */
#include "PetriEngine/Structures/StateSet.h"
#include "LTL/Structures/ProductState.h"
#include "BitProductStateSet.h"

#include <ptrie/ptrie.h>
#include <cstdint>

#ifndef COMPOUNDSTATESET_H
#define COMPOUNDSTATESET_H
namespace LTL { namespace Structures {
    template<typename stateset_type = ptrie::set<stateid_t,17,32,8>, uint8_t nbits = 20>
    class CompoundStateSet {
    public:

        explicit CompoundStateSet(const PetriEngine::PetriNet& net, size_t traces, uint32_t kbound = 0)
                : _markings(net, kbound, net.numberOfPlaces()), _hyper_traces(traces)
        {
            _scratchpad = std::make_unique<size_t[]>(_hyper_traces);
        }

        static_assert(nbits <= 32, "Only up to 2^32 Büchi states supported");
        static_assert(sizeof(size_t) >= 8, "Expecting size_t to be at least 8 bytes");

        static size_t get_buchi_state(stateid_t id) { return id & BUCHI_MASK; }

        static size_t get_marking_id(stateid_t id) { return id >> MARKING_SHIFT; }

        static stateid_t get_product_id(size_t markingId, size_t buchiState)
        {
            return (buchiState & BUCHI_MASK) | (markingId << MARKING_SHIFT);
        }

        /**
         * Insert a product state into the state set.
         * @param state the product state to insert.
         * @return tripple of [success, ID, data_id] where data_id can be used
         *         for quick-access via get_data (constant-time lookup).
         */
        result_t add(const LTL::Structures::ProductState &state)
        {
            ++_discovered;
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                PetriEngine::Structures::State dummy(const_cast<PetriEngine::MarkVal*>(state.marking()) + i * _markings.net().numberOfPlaces());
                const auto res = _markings.add(dummy);
                dummy.release();
                if (res.second == std::numeric_limits<size_t>::max()) {
                    return {res.first, res.second, res.second};
                }
                _scratchpad[i] = res.second;
            }
            auto res = _compounds.insert(_scratchpad.get(), _hyper_traces);
            const stateid_t product_id = get_product_id(res.second, state.get_buchi_state());
            assert(res.second == get_marking_id(product_id));
            assert(state.get_buchi_state() == get_buchi_state(product_id));
            auto [is_new, data_id] = _states.insert(product_id);
            return {is_new, product_id, data_id};
        }

        /**
         * Gets the associated data of a data_id, constants-time
         * @param data_id
         * @return the data-object pointed to by data_id (obtained by the add() method).
         */
        auto& get_data(size_t data_id) {
            return _states.get_data(data_id);
        }

        /**
         * Retrieve a product state from the state set.
         * @param id Composite state ID as previously generated by this.
         * @param state Output parameter to write product state to.
         * @return true if the state was successfully retrieved, false otherwise.
         */
        virtual void decode(LTL::Structures::ProductState &state, stateid_t id)
        {
            assert(_states.exists(id).first);
            auto marking_id = get_marking_id(id);
            _compounds.unpack(marking_id, _scratchpad.get());
            auto buchi_state = get_buchi_state(id);
            for(size_t i = 0; i < _hyper_traces; ++i)
            {
                PetriEngine::Structures::State dummy(state.marking() + i * _markings.net().numberOfPlaces());
                _markings.decode(dummy, _scratchpad[i]);
                dummy.release();
            }
            state.set_buchi_state(buchi_state);
        }

        size_t discovered() const { return _discovered; }

        size_t max_tokens() const { return _markings.maxTokens(); }

    protected:

        static constexpr auto BUCHI_MASK = ~(std::numeric_limits<size_t>::max() << (nbits));
        static constexpr auto MARKING_SHIFT = nbits;

        PetriEngine::Structures::StateSet _markings;
        stateset_type _states;
        ptrie::set_stable<size_t,size_t,17,128,4> _compounds;
        static constexpr auto _err_val = std::make_pair(false, std::numeric_limits<size_t>::max());

        size_t _discovered = 0;
        const size_t _hyper_traces;
        std::unique_ptr<size_t[]> _scratchpad;
    };

    template<uint8_t nbits = 20>
    class TraceableCompoundStateSet : public CompoundStateSet<ptrie::map<stateid_t,std::pair<size_t,size_t>>, nbits> {
    public:
        explicit TraceableCompoundStateSet(const PetriEngine::PetriNet& net, size_t traces, uint32_t kbound = 0)
                : BitProductStateSet<ptrie::map<stateid_t,std::pair<size_t,size_t>>,nbits>(net, traces, kbound)
        { }

        void decode(ProductState &state, stateid_t id) override
        {
            _parent = id;
            BitProductStateSet<ptrie::map<stateid_t,std::pair<size_t,size_t>>,nbits>::decode(state, id);
        }

        void set_history(stateid_t id, size_t transition)
        {
            assert(this->_states.exists(id).first);
            this->_states[id] = {_parent, transition};
        }

        std::pair<size_t, size_t> get_history(stateid_t stateid)
        {
            assert(this->_states.exists(stateid).first);
            return this->_states[stateid];
        }

    private:
        stateid_t _parent = 0;
    };
} }
#endif /* COMPOUNDSTATESET_H */

