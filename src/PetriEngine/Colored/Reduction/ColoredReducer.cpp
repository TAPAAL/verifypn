/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {

    std::vector<ApplicationSummary> ColoredReducer::createApplicationSummary() const {
        std::vector<ApplicationSummary> res;
        for (auto &rule : _reductions) {
            res.emplace_back(rule->name(), rule->applications());
        }
        std::sort(res.begin(), res.end());
        return res;
    }

    bool ColoredReducer::reduce(uint32_t timeout, const std::vector<bool> &inQuery, bool preserveDeadlocks) {

        _startTime = std::chrono::high_resolution_clock::now();
        if (timeout <= 0) return false;
        _timeout = timeout;

        bool any = false;
        bool changed;
        do {
            changed = false;

            for (auto &rule: _reductions) {
                if (rule->canBeAppliedRepeatedly())
                    while (rule->apply(*this, inQuery, preserveDeadlocks)) changed = true;
                else
                    changed |= rule->apply(*this, inQuery, preserveDeadlocks);
            }

            any |= changed;
        } while (changed && hasTimedOut());

        auto now = std::chrono::high_resolution_clock::now();
        _timeSpent = (std::chrono::duration_cast<std::chrono::microseconds>(now - _startTime).count()) * 0.000001;
        return any;
    }

    CArcIter ColoredReducer::getInArc(uint32_t pid, Colored::Transition &tran) const {
        return std::find_if(tran.input_arcs.begin(), tran.input_arcs.end(),
                            [&pid](Colored::Arc &arc) { return arc.place == pid; });
    }

    CArcIter ColoredReducer::getOutArc(Colored::Transition &tran, uint32_t pid) const {
        return std::find_if(tran.output_arcs.begin(), tran.output_arcs.end(),
                            [&pid](Colored::Arc &arc) { return arc.place == pid; });
    }

    void ColoredReducer::skipPlace(uint32_t pid) {
        Place &place = _builder._places[pid];
        assert(!place.skipped);
        place.skipped = true;
        _skippedPlaces.push_back(pid);
        for (auto &tid: place._pre) {
            Transition &tran = _builder._transitions[tid];
            auto ait = getOutArc(tran, pid);
            assert(ait != tran.output_arcs.end());
            tran.output_arcs.erase(ait);
        }
        for (auto &tid: place._post) {
            Transition &tran = _builder._transitions[tid];
            auto ait = getInArc(pid, tran);
            assert(ait != tran.input_arcs.end());
            tran.input_arcs.erase(ait);
        }
        auto &inhibs = _builder._inhibitorArcs;
        inhibs.erase(std::remove_if(inhibs.begin(), inhibs.end(),
                                    [&pid](Arc &arc) { return arc.place == pid; }),
                     inhibs.end());
        place._post.clear();
        place._pre.clear();
    }

    void ColoredReducer::skipTransition(uint32_t tid) {
        Transition &tran = _builder._transitions[tid];
        assert(!tran.skipped);
        tran.skipped = true;
        _skippedTransitions.push_back(tid);
        for (auto &pid: tran.input_arcs) {
            Place &place = _builder._places[pid.place];
            auto it = std::lower_bound(place._post.begin(), place._post.end(), tid);
            assert(it != place._post.end());
            place._post.erase(it);
        }
        for (auto &pid: tran.output_arcs) {
            Place &place = _builder._places[pid.place];
            auto it = std::lower_bound(place._pre.begin(), place._pre.end(), tid);
            assert(it != place._pre.end());
            place._pre.erase(it);
        }
        auto &inhibs = _builder._inhibitorArcs;
        inhibs.erase(std::remove_if(inhibs.begin(), inhibs.end(),
                                    [&tid](Arc &arc) { return arc.transition == tid; }),
                     inhibs.end());
        tran.input_arcs.clear();
        tran.output_arcs.clear();
    }
}