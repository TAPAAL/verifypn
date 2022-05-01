

#include "PetriEngine/Colored/ForwardFixedPoint.h"
#include "PetriEngine/AbstractPetriNetBuilder.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/VariableVisitor.h"
#include "PetriEngine/Colored/ArcIntervalVisitor.h"
#include "PetriEngine/Colored/RestrictVisitor.h"
#include "PetriEngine/Colored/OutputIntervalVisitor.h"

#include <chrono>

namespace PetriEngine {
    namespace Colored {

        void ForwardFixedPoint::printPlaceTable() const {

            for (const auto &place : _builder.places()) {
                const auto &placeID = _builder.colored_placenames().find(place.name)->second;
                const auto &placeColorFixpoint = _placeColorFixpoints[placeID];
                std::cout << "Place: " << place.name << " in queue: " << placeColorFixpoint.inQueue << " with colortype " << place.type->getName() << std::endl;

                for (const auto &fixpointPair : placeColorFixpoint.constraints) {
                    std::cout << "[";
                    for (const auto &range : fixpointPair._ranges) {
                        std::cout << range._lower << "-" << range._upper << ", ";
                    }
                    std::cout << "]" << std::endl;
                }
                std::cout << std::endl;
            }
        }


        //Create Arc interval structures for the transition

        std::unordered_map<uint32_t, Colored::ArcIntervals> ForwardFixedPoint::setupTransitionVars(size_t tid) const {
            std::unordered_map<uint32_t, Colored::ArcIntervals> res;
            for (auto arc : _builder.transitions()[tid].input_arcs) {
                std::set<const Colored::Variable *> variables;
                Colored::PositionVariableMap varPositions;
                Colored::VariableModifierMap varModifiersMap;
                Colored::VariableVisitor::get_variables(*arc.expr, variables, varPositions, varModifiersMap, false);

                res.emplace(std::make_pair(arc.place, Colored::ArcIntervals(std::move(varModifiersMap))));
            }
            return res;
        }

        void ForwardFixedPoint::add_place(const Colored::Place& place) {
            Colored::interval_vector_t placeConstraints;
            Colored::ColorFixpoint colorFixpoint = {placeConstraints, !place.marking.empty()};
            uint32_t colorCounter = 0;

            if (place.marking.size() == place.type->size()) {
                for (const auto& colorPair : place.marking) {
                    if (colorPair.second > 0) {
                        colorCounter++;
                    } else {
                        break;
                    }
                }
            }

            if (colorCounter == place.type->size()) {
                colorFixpoint.constraints.addInterval(place.type->getFullInterval());
            } else {
                for (const auto& colorPair : place.marking) {
                    Colored::interval_t tokenConstraints;
                    uint32_t index = 0;
                    colorPair.first->getColorConstraints(tokenConstraints, index);

                    colorFixpoint.constraints.addInterval(tokenConstraints);
                }
            }

            _placeColorFixpoints.push_back(colorFixpoint);
        }

        void ForwardFixedPoint::init() {
            auto& transitions = _builder.transitions();
            auto& places = _builder.places();
            _placeColorFixpoints.clear();
            for(auto& p : places)
                add_place(p);
            _transition_variable_maps.clear();
            _transition_variable_maps.resize(transitions.size());
            _arcIntervals.clear();
            _arcIntervals.resize(transitions.size());
            for (size_t t = 0; t < transitions.size(); ++t)
                _arcIntervals[t] = setupTransitionVars(t);
        }

        void ForwardFixedPoint::set_default() {

            auto& transitions = _builder.transitions();
            auto& places = _builder.places();
            init();
            for (uint32_t transitionId = 0; transitionId < transitions.size(); transitionId++) {
                const Colored::Transition &transition = transitions[transitionId];
                std::set<const Colored::Variable *> variables;

                for (const auto &inArc : transition.input_arcs) {
                    Colored::ArcIntervals& arcInterval = _arcIntervals[transitionId][inArc.place];
                    arcInterval._intervalTupleVec.clear();

                    Colored::interval_vector_t intervalTuple;
                    intervalTuple.addInterval(places[inArc.place].type->getFullInterval());
                    const PetriEngine::Colored::ColorFixpoint cfp{intervalTuple};

                    Colored::ArcIntervalVisitor::intervals(*inArc.expr, arcInterval, cfp);
                    if(_partition.computed())
                        _partition.partition()[inArc.place].applyPartition(arcInterval);
                }

                IntervalGenerator::getVarIntervals(_transition_variable_maps[transitionId], _arcIntervals[transitionId]);
                for (const auto &outArc : transition.output_arcs)
                    Colored::VariableVisitor::get_variables(*outArc.expr, variables);
                if (transition.guard != nullptr) {
                    Colored::VariableVisitor::get_variables(*transition.guard, variables);
                }
                for (auto* var : variables) {
                    for (auto& varmap : _transition_variable_maps[transitionId]) {
                        if (varmap.count(var) == 0) {
                            Colored::interval_vector_t intervalTuple;
                            intervalTuple.addInterval(var->colorType->getFullInterval());
                            varmap[var] = intervalTuple;
                        }
                    }
                }
            }
        }

        void ForwardFixedPoint::compute(uint32_t maxIntervals, uint32_t maxIntervalsReduced, int32_t timeout) {
            if (_builder.isColored()) {
                init();
                auto& places = _builder.places();
                auto& transitions = _builder.transitions();
                for (size_t i = 0; i < places.size(); ++i) {
                    if (places[i].skipped) continue;
                    _placeFixpointQueue.emplace_back(i);
                }
                _considered.resize(transitions.size());
                std::fill(_considered.begin(), _considered.end(), false);

                //Start timers for timing color fixpoint creation and max interval reduction steps
                auto start = std::chrono::high_resolution_clock::now();
                auto end = std::chrono::high_resolution_clock::now();
                auto reduceTimer = std::chrono::high_resolution_clock::now();
                while (!_placeFixpointQueue.empty()) {
                    //Reduce max interval once timeout passes
                    if (maxIntervals > maxIntervalsReduced && timeout > 0 && std::chrono::duration_cast<std::chrono::seconds>(end - reduceTimer).count() >= timeout) {
                        maxIntervals = maxIntervalsReduced;
                    }

                    uint32_t currentPlaceId = _placeFixpointQueue.back();
                    _placeFixpointQueue.pop_back();
                    _placeColorFixpoints[currentPlaceId].inQueue = false;

                    for (auto transitionId : places[currentPlaceId]._post) {
                        const Colored::Transition& transition = _builder.transitions()[transitionId];
                        // Skip transitions that cannot add anything new,
                        // such as transitions with only constants on their arcs that have been processed once
                        assert(transitionId < _builder.transitions().size());
                        assert(transitionId < _considered.size());
                        if (_considered[transitionId]) continue;
                        bool transitionActivated = true;
                        _transition_variable_maps[transitionId].clear();

                        processInputArcs(transition, currentPlaceId, transitionId, transitionActivated, maxIntervals);

                        //If there were colors which activated the transitions, compute the intervals produced
                        if (transitionActivated) {
                            processOutputArcs(transition, transitionId);
                        }
                    }
                    end = std::chrono::high_resolution_clock::now();
                }

                _fixpointDone = true;
                _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
            }
        }

        //Retreive interval colors from the input arcs restricted by the transition guard

        void ForwardFixedPoint::processInputArcs(const Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals) {
            getArcIntervals(transition, transitionActivated, max_intervals, transitionId);

            if (!transitionActivated) {
                return;
            }
            if (IntervalGenerator::getVarIntervals(_transition_variable_maps[transitionId], _arcIntervals[transitionId])) {
                if (transition.guard != nullptr) {
                    addTransitionVars(transitionId);
                    Colored::RestrictVisitor::restrict(*transition.guard, _transition_variable_maps[transitionId]);
                    removeInvalidVarmaps(transitionId);

                    if (_transition_variable_maps[transitionId].empty()) {
                        //Guard restrictions removed all valid intervals
                        transitionActivated = false;
                        return;
                    }
                }
            } else {
                //Retrieving variable intervals failed
                transitionActivated = false;
            }
        }

        void ForwardFixedPoint::getArcIntervals(const Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId) {
            for (auto& arc : transition.input_arcs) {
                PetriEngine::Colored::ColorFixpoint& curCFP = _placeColorFixpoints[arc.place];
                curCFP.constraints.restrict(max_intervals);
                _max_intervals = std::max(_max_intervals, curCFP.constraints.size());
                assert(_arcIntervals.size() >= transitionId);
                Colored::ArcIntervals& arcInterval = _arcIntervals[transitionId][arc.place];
                arcInterval._intervalTupleVec.clear();

                if (!Colored::ArcIntervalVisitor::intervals(*arc.expr, arcInterval, curCFP)) {
                    transitionActivated = false;
                    return;
                }

                if (_partition.computed()) {
                    _partition.partition()[arc.place].applyPartition(arcInterval);
                }
            }

        }

        void ForwardFixedPoint::addTransitionVars(size_t transition_id) {
            std::set<const Colored::Variable *> variables;
            Colored::VariableVisitor::get_variables(*_builder.transitions()[transition_id].guard, variables);
            for (auto* var : variables) {
                for (auto& varmap : _transition_variable_maps[transition_id]) {
                    if (varmap.count(var) == 0) {
                        Colored::interval_vector_t intervalTuple;
                        intervalTuple.addInterval(var->colorType->getFullInterval());
                        varmap[var] = std::move(intervalTuple);
                    }
                }
            }
        }

        void ForwardFixedPoint::removeInvalidVarmaps(size_t tid) {
            std::vector<Colored::VariableIntervalMap> newVarmaps;
            for (auto& varMap : _transition_variable_maps[tid]) {
                bool validVarMap = true;
                for (auto& varPair : varMap) {
                    if (!varPair.second.hasValidIntervals()) {
                        validVarMap = false;
                        break;
                    } else {
                        varPair.second.simplify();
                    }
                }
                if (validVarMap) {
                    newVarmaps.push_back(std::move(varMap));
                }
            }
            _transition_variable_maps[tid] = std::move(newVarmaps);
        }

        void ForwardFixedPoint::processOutputArcs(const Colored::Transition& transition, size_t transition_id) {
            bool transitionHasVarOutArcs = false;
            for (const auto& arc : transition.output_arcs) {
                Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
                //used to check if colors are added to the place. The total distance between upper and
                //lower bounds should grow when more colors are added and as we cannot remove colors this
                //can be checked by summing the differences
                uint32_t colorsBefore = placeFixpoint.constraints.getContainedColors();

                std::set<const Colored::Variable *> variables;
                Colored::VariableVisitor::get_variables(*arc.expr, variables);

                if (!variables.empty()) {
                    transitionHasVarOutArcs = true;
                }



                //Apply partitioning to unbound outgoing variables such that
                // bindings are only created for colors used in the rest of the net
                if (_partition.computed() && !_partition.partition()[arc.place].isDiagonal()) {
                    for (auto* outVar : variables) {
                        for (auto& varMap : _transition_variable_maps[transition_id]) {
                            if (varMap.count(outVar) == 0) {
                                Colored::interval_vector_t varIntervalTuple;
                                for (const auto& EqClass : _partition.partition()[arc.place].getEquivalenceClasses()) {
                                    varIntervalTuple.addInterval(EqClass.intervals().back().getSingleColorInterval());
                                }
                                varMap[outVar] = std::move(varIntervalTuple);
                            }
                        }
                    }
                } else {
                    // Else if partitioning was not computed or diagonal
                    // and there is a varaible which was not found on an input arc or in the guard,
                    // we give it the full interval
                    for (auto* var : variables) {
                        for (auto& varmap : _transition_variable_maps[transition_id]) {
                            if (varmap.count(var) == 0) {
                                Colored::interval_vector_t intervalTuple;
                                intervalTuple.addInterval(var->colorType->getFullInterval());
                                varmap[var] = std::move(intervalTuple);
                            }
                        }
                    }
                }

                auto intervals = Colored::OutputIntervalVisitor::intervals(*arc.expr, _transition_variable_maps[transition_id]);


                for (auto& intervalTuple : intervals) {
                    intervalTuple.simplify();
                    for (auto& interval : intervalTuple) {
                        placeFixpoint.constraints.addInterval(std::move(interval));
                    }
                }
                placeFixpoint.constraints.simplify();

                //Check if the place should be added to the queue
                if (!placeFixpoint.inQueue) {
                    uint32_t colorsAfter = placeFixpoint.constraints.getContainedColors();
                    if (colorsAfter > colorsBefore) {
                        _placeFixpointQueue.push_back(arc.place);
                        placeFixpoint.inQueue = true;
                    }
                }
            }
            //If there are no variables among the out arcs of a transition
            // and it has been activated, there is no reason to cosider it again
            if (!transitionHasVarOutArcs) {
                _considered[transition_id] = true;
            }
        }

    }
}