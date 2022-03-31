/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {

    ColoredReducer::ColoredReducer(PetriEngine::ColoredPetriNetBuilder &b) : _builder(b),
                                                                             _origPlaceCount(b.getPlaceCount()),
                                                                             _origTransitionCount(
                                                                                     b.getTransitionCount()) {
        b.sort();

#ifndef NDEBUG
        // All rule names must be unique
        std::set<std::string> names;
        for (auto &rule : _reductions) {
            assert(names.find(rule->name()) == names.end());
            names.insert(rule->name());
        }
        consistent();
#endif
    }

    std::vector<ApplicationSummary> ColoredReducer::createApplicationSummary() const {
        std::vector<ApplicationSummary> res;
        for (auto &rule : _reductions) {
            res.emplace_back(rule->name(), rule->applications());
        }
        std::sort(res.begin(), res.end());
        return res;
    }

    bool ColoredReducer::reduce(uint32_t timeout, const std::vector<bool> &inQuery, QueryType queryType,
                                bool preserveLoops, bool preserveStutter, uint32_t reductiontype,
                                std::vector<uint32_t> &reductions) {

        _startTime = std::chrono::high_resolution_clock::now();
        if (timeout <= 0) return false;
        _timeout = timeout;

        bool any = false;
        bool changed;

        std::vector<ReductionRule *> reductionsToUse;

        if (reductiontype == 2) {
            reductionsToUse = ColoredReducer::buildApplicationSequence(reductions);
        } else {
            reductionsToUse = _reductions;
        }
        for (int i = reductionsToUse.size() - 1; i >= 0; i--) {
            if (!reductionsToUse[i]->isApplicable(queryType, preserveLoops, preserveStutter))
                reductionsToUse.erase(reductionsToUse.begin() + i);
        }

        do {
            changed = false;
            for (auto &rule: reductionsToUse) {
                changed |= rule->apply(*this, inQuery, queryType, preserveLoops, preserveStutter);
            }
            any |= changed;
        } while (changed && hasTimedOut());

        auto now = std::chrono::high_resolution_clock::now();
        _timeSpent = (std::chrono::duration_cast<std::chrono::microseconds>(now - _startTime).count()) * 0.000001;
        return any;
    }

    CArcIter ColoredReducer::getInArc(uint32_t pid, const Colored::Transition &tran) const {
        return std::find_if(tran.input_arcs.begin(), tran.input_arcs.end(),
                            [&pid](const Colored::Arc &arc) { return arc.place == pid; });
    }

    CArcIter ColoredReducer::getOutArc(const Colored::Transition &tran, uint32_t pid) const {
        return std::find_if(tran.output_arcs.begin(), tran.output_arcs.end(),
                            [&pid](const Colored::Arc &arc) { return arc.place == pid; });
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

    void ColoredReducer::consistent() {
#ifndef NDEBUG
        uint32_t skippedPlaces = 0;
        for (uint32_t p = 0; p < _builder._places.size(); p++) {
            const Place &place = _builder._places[p];
            if (place.skipped) {
                skippedPlaces++;
                assert(std::find(_skippedPlaces.begin(), _skippedPlaces.end(), p) != _skippedPlaces.end());
                assert(place._pre.empty());
                assert(place._post.empty());
            }
            if (place.inhibitor) {
                bool found = false;
                for (const Arc &arc : _builder._inhibitorArcs) {
                    if (arc.place == p) {
                        found = true;
                        break;
                    }
                }
                assert(found);
            }
            assert(std::is_sorted(place._pre.begin(), place._pre.end()));
            assert(std::is_sorted(place._post.begin(), place._post.end()));

            for (uint32_t t : place._pre) {
                Transition &tran = _builder._transitions[t];
                assert(!tran.skipped);
                auto arc = getOutArc(tran, p);
                assert(arc != tran.output_arcs.end());
                assert(arc->place == p);
            }

            for (uint32_t t : place._post) {
                Transition &tran = _builder._transitions[t];
                assert(!tran.skipped);
                auto arc = getInArc(p, tran);
                assert(arc != tran.input_arcs.end());
                assert(arc->place == p);
            }
        }
        assert(skippedPlaces == _skippedPlaces.size());

        uint32_t skippedTransitions = 0;
        for (uint32_t t = 0; t < _builder._transitions.size(); t++) {
            const Transition &tran = _builder._transitions[t];
            if (tran.skipped) {
                skippedTransitions++;
                assert(std::find(_skippedTransitions.begin(), _skippedTransitions.end(), t) != _skippedTransitions.end());
                assert(tran.input_arcs.empty());
                assert(tran.output_arcs.empty());
            }
            if (tran.inhibited) {
                bool found = false;
                for (const Arc &arc : _builder._inhibitorArcs) {
                    if (arc.transition == t) {
                        found = true;
                        break;
                    }
                }
                assert(found);
            }

            int32_t prevPlace = -1;
            for (const Arc &arc : tran.input_arcs) {
                assert((int32_t)arc.place > prevPlace);
                prevPlace = arc.place;
                Place &place = _builder._places[arc.place];
                assert(!place.skipped);
                assert(arc.expr != nullptr);
                assert(arc.inhib_weight == 0);
                assert(std::find(place._post.begin(), place._post.end(), t) != place._post.end());
            }
            prevPlace = -1;
            for (const Arc &arc : tran.output_arcs) {
                assert((int32_t)arc.place > prevPlace);
                prevPlace = arc.place;
                Place &place = _builder._places[arc.place];
                assert(!place.skipped);
                assert(arc.expr != nullptr);
                assert(arc.inhib_weight == 0);
                assert(std::find(place._pre.begin(), place._pre.end(), t) != place._pre.end());
            }
        }
        assert(skippedTransitions == _skippedTransitions.size());

        for (const Arc &arc : _builder._inhibitorArcs) {
            assert(arc.inhib_weight > 0);
            assert(arc.expr == nullptr);
            assert(_builder._places[arc.place].inhibitor);
            assert(_builder._transitions[arc.transition].inhibited);
        }
#endif
    }

    std::string ColoredReducer::newTransitionName()
    {
        auto prefix = "CCT";
        auto tmp = prefix + std::to_string(_tnameid);
        while(_builder._transitionnames.count(tmp) >= 1)
        {
            ++_tnameid;
            tmp = prefix + std::to_string(_tnameid);
        }
        return tmp;
    }

    uint32_t ColoredReducer::newTransition(const Colored::GuardExpression_ptr& guard){
        uint32_t id = transitions().size();
        if (!_skippedTransitions.empty())
        {
            id = _skippedTransitions.back();
            _skippedTransitions.pop_back();
            PetriEngine::Colored::Transition& tran = _builder._transitions[id];
            tran.guard = nullptr;
            tran.skipped = false;
            tran.inhibited = false;
            _builder._transitionnames.erase(tran.name);
            tran.name = newTransitionName();
            _builder._transitionnames[tran.name] = id;
        }
        else
        {
            _builder.addTransition(newTransitionName(), guard, 0,0,0);
        }
        return id;
    }

    void ColoredReducer::addInputArc(uint32_t pid, uint32_t tid, ArcExpression_ptr& expr, uint32_t inhib_weight){
        _builder.addInputArc(_builder._places[pid].name, _builder._transitions[tid].name, expr, inhib_weight);
        std::sort(_builder._places[pid]._post.begin(), _builder._places[pid]._post.end());
        std::sort(_builder._transitions[tid].input_arcs.begin(), _builder._transitions[tid].input_arcs.end(), ArcLessThanByPlace);
    }
    void ColoredReducer::addOutputArc(uint32_t tid, uint32_t pid, ArcExpression_ptr expr){
        _builder.addOutputArc(_builder._transitions[tid].name, _builder._places[pid].name, expr);
        std::sort(_builder._places[pid]._pre.begin(), _builder._places[pid]._pre.end());
        std::sort(_builder._transitions[tid].output_arcs.begin(), _builder._transitions[tid].output_arcs.end(), ArcLessThanByPlace);
    }

}