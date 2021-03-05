#include "PetriEngine/ReducingSuccessorGenerator.h"

#include <cassert>
#include <utility>
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet &net,
                                                           std::shared_ptr<StubbornSet> stubbornSet)
            : SuccessorGenerator(net), _stubSet(std::move(stubbornSet)) { }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet &net,
                                                           std::shared_ptr<StubbornSet> stubbornSet,
                                                           const bool* enabled, const bool* stubborn)
            : SuccessorGenerator(net), _stubSet(std::move(stubbornSet)), _enabled(enabled), _stubborn(stubborn) { }

    void ReducingSuccessorGenerator::reset() {
        SuccessorGenerator::reset();
        _stubSet->reset();
    }

    bool ReducingSuccessorGenerator::next(Structures::State &write) {
        _current = _stubSet->next();
        if (_current == std::numeric_limits<uint32_t>::max()) {
            reset();
            return false;
        }
        _fire(write, _current);
        return true;
    }

    bool ReducingSuccessorGenerator::next(Structures::State &write, sucinfo &sucinfo)
    {
/*            if (sucinfo.tid == sucinfo::no_value)
½                return false;*/
        assert(_enabled != nullptr && _stubborn != nullptr);

        if (sucinfo.tid == std::numeric_limits<uint32_t>::max()) {
            sucinfo.tid = 0;
        }
        while (sucinfo.tid < _net._ntransitions) {
            if (_enabled[sucinfo.tid] && _stubborn[sucinfo.tid]) {
                assert(checkPreset(sucinfo.tid));
                _fire(write, sucinfo.tid);
                sucinfo.tid++;
                return true;
            }
            sucinfo.tid++;
        }
        return false;

       /* if (sucinfo.tid >= _net.numberOfTransitions()) {
            return false;
        }
        assert(checkPreset(sucinfo.tid));
        _fire(write, sucinfo.tid);
        do {
            ++sucinfo.tid;
        } while (sucinfo.tid < _net.numberOfTransitions() && (!_enabled[sucinfo.tid] || !_stubborn[sucinfo.tid]));

        return true;*/
    }

    bool ReducingSuccessorGenerator::prepare(const Structures::State *state) {
        _current = 0;
        _parent = state;
        return _stubSet->prepare(state);
    }


}
