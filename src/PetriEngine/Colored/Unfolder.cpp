#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/EvaluationVisitor.h"
#include "PetriEngine/Colored/Unfolder.h"
#include "PetriEngine/Colored/BindingGenerator.h"

namespace PetriEngine {
    namespace Colored {

        std::string Unfolder::arc_to_string(const Colored::Arc& arc) const {
            return !arc.input ? "(" + *_builder.transitions()[arc.transition].name + ", " + *_builder.places()[arc.place].name + ")" :
                "(" + *_builder.places()[arc.place].name + ", " + *_builder.transitions()[arc.transition].name + ")";
        }

        PetriNetBuilder Unfolder::strip_colors() {
            PetriNetBuilder pnBuilder(_builder.string_set());
            if (_builder.isColored()) {
                for (auto& place : _builder.places()) {
                    if (place.skipped) continue;
                    pnBuilder.addPlace(place.name, place.marking.size(), place._x, place._y);
                }

                for (auto& transition : _builder.transitions()) {
                    if (transition.skipped) continue;
                    pnBuilder.addTransition(transition.name, transition._player, transition._x, transition._y);
                    for (const auto& arc : transition.input_arcs) {
                        try {
                            pnBuilder.addInputArc(_builder.places()[arc.place].name, _builder.transitions()[arc.transition].name, false,
                                arc.expr->weight());
                        } catch (base_error& e) {
                            std::stringstream ss;
                            ss << "Exception on input arc: " << arc_to_string(arc) << std::endl;
                            ss << "In expression: " << *arc.expr << std::endl;
                            ss << e.what() << std::endl;
                            throw base_error(ss.str());
                        }
                    }
                    for (const auto& arc : transition.output_arcs) {
                        try {
                            pnBuilder.addOutputArc(_builder.transitions()[arc.transition].name, _builder.places()[arc.place].name,
                                arc.expr->weight());
                        } catch (base_error& e) {
                            std::stringstream ss;
                            ss << "Exception on output arc: " << arc_to_string(arc) << std::endl;
                            ss << "In expression: " << *arc.expr << std::endl;
                            ss << e.what() << std::endl;
                            throw base_error(ss.str());
                        }
                    }
                    for (const auto& arc : _builder.inhibitors()) {
                        pnBuilder.addInputArc(_builder.places()[arc.place].name, _builder.transitions()[arc.transition].name, true,
                            arc.inhib_weight);
                    }
                }
            }

            return pnBuilder;
        }

        PetriNetBuilder Unfolder::unfold() {
            PetriNetBuilder ptBuilder(_builder.string_set());
            if (_builder.isColored()) {
                auto start = std::chrono::high_resolution_clock::now();

                for (uint32_t transitionId = 0; transitionId < _builder.transitions().size(); transitionId++) {
                    unfoldTransition(ptBuilder, transitionId);
                }

                const auto& unfoldedPlaceMap = ptBuilder.getPlaceNames();
                for (auto& place : _builder.places()) {
                    if (place.skipped) continue;
                    handleOrphanPlace(ptBuilder, place, unfoldedPlaceMap);
                }

                auto end = std::chrono::high_resolution_clock::now();
                _time = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
            }
            return ptBuilder;
        }

        //Due to the way we unfold places, we only unfold places connected to an arc (which makes sense)
        //However, in queries asking about orphan places it cannot find these, as they have not been unfolded
        //so we make a placeholder place which just has tokens equal to the number of colored tokens
        //Ideally, orphan places should just be translated to a constant in the query

        void Unfolder::handleOrphanPlace(PetriNetBuilder& ptBuilder, const Colored::Place& place,
            const shared_name_index_map& unfoldedPlaceMap) {
            if (_ptplacenames.count(place.name) <= 0 && place.marking.size() > 0) {
                auto name = std::make_shared<const_string>(*place.name + "_orphan");
                ptBuilder.addPlace(name, place.marking.size(), place._x, place._y);
                _ptplacenames[place.name][0] = std::move(name);
            } else {
                uint32_t usedTokens = 0;
                bool any = false;
                for (const auto &unfoldedPlace : _ptplacenames[place.name]) {
                    any = true;
                    auto unfoldedMarking = ptBuilder.initMarking();
                    usedTokens += unfoldedMarking[unfoldedPlaceMap.find(unfoldedPlace.second)->second];
                }

                if (place.marking.size() > usedTokens || !any) {
                    auto name = std::make_shared<const_string>(*place.name + "_orphan");
                    ptBuilder.addPlace(name, place.marking.size() - usedTokens, place._x, place._y);
                    _ptplacenames[place.name][std::numeric_limits<uint32_t>::max()] = std::move(name);
                }
            }
        }

        void Unfolder::unfoldPlace(PetriNetBuilder& ptBuilder, const Colored::Place* place, const PetriEngine::Colored::Color *color, uint32_t placeId, uint32_t id) {
            size_t tokenSize = 0;
            if (!_partition.computed() || _partition.partition()[placeId].isDiagonal()) {
                tokenSize = place->marking[color];
            } else {
                const std::vector<const Colored::Color*>& tupleColors = color->getTupleColors();
                const size_t &tupleSize = _partition.partition()[placeId].getDiagonalTuplePositions().size();
                const uint32_t &classId = _partition.partition()[placeId].getColorEqClassMap().find(color)->second->id();
                const auto &diagonalTuplePos = _partition.partition()[placeId].getDiagonalTuplePositions();

                for (const auto &colorEqClassPair : _partition.partition()[placeId].getColorEqClassMap()) {
                    if (colorEqClassPair.second->id() == classId) {
                        const std::vector<const Colored::Color*>& testColors = colorEqClassPair.first->getTupleColors();
                        bool match = true;
                        for (uint32_t i = 0; i < tupleSize; i++) {
                            if (diagonalTuplePos[i] && tupleColors[i]->getId() != testColors[i]->getId()) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            tokenSize += place->marking[colorEqClassPair.first];
                        }
                    }
                }
            }
            auto name = std::make_shared<const_string>(*place->name + "_" + std::to_string(color->getId()));

            ptBuilder.addPlace(name, tokenSize, place->_x, place->_y + (15 * color->getId()));
            _ptplacenames[place->name][id] = std::move(name);
        }

        void Unfolder::unfoldTransition(PetriNetBuilder& ptBuilder, uint32_t transitionId) {
            double offset = 0;
            const Colored::Transition &transition = _builder.transitions()[transitionId];
            if (transition.skipped) return;
            if (_fixed_point.computed() || _partition.computed()) {
                assert(_fixed_point.variable_map().size() > transitionId);
                assert(_symmetry.symmetries().size() > transitionId);
                FixpointBindingGenerator gen(transition, _builder.colors(), _symmetry.symmetries()[transitionId],
                    _fixed_point.variable_map()[transitionId]);
                size_t i = 0;
                bool hasBindings = false;
                for (const auto &b : gen) {
                    auto name = std::make_shared<const_string>(*transition.name + "_" + std::to_string(i++));

                    storeBinding(name, b);
                   
                    hasBindings = true;
                    ptBuilder.addTransition(name, transition._player, transition._x, transition._y + offset);
                    offset += 15;

                    for (auto& arc : transition.input_arcs) {
                        unfoldArc(ptBuilder, arc, b, name);
                    }
                    for (auto& arc : transition.output_arcs) {
                        unfoldArc(ptBuilder, arc, b, name);
                    }

                    _pttransitionnames[transition.name].push_back(name);
                    unfoldInhibitorArc(ptBuilder, transition.name, name);
                }
                if (!hasBindings) {
                    _pttransitionnames[transition.name] = std::vector<shared_const_string>();
                }
            } else {
                NaiveBindingGenerator gen(transition, _builder.colors());
                size_t i = 0;
                for (const auto &b : gen) {
                    auto name = std::make_shared<const_string>(*transition.name + "_" + std::to_string(i++));
                    ptBuilder.addTransition(name, transition._player, transition._x, transition._y + offset);
                    offset += 15;

                    storeBinding(name, b);

                    for (const auto& arc : transition.input_arcs) {
                        unfoldArc(ptBuilder, arc, b, name);
                    }
                    for (const auto& arc : transition.output_arcs) {
                        unfoldArc(ptBuilder, arc, b, name);
                    }
                    _pttransitionnames[transition.name].push_back(name);
                    unfoldInhibitorArc(ptBuilder, transition.name, name);
                }
            }
        }

        void Unfolder::unfoldInhibitorArc(PetriNetBuilder& ptBuilder, const shared_const_string &oldname, const shared_const_string &newname) {
            for (uint32_t i = 0; i < _builder.inhibitors().size(); ++i) {
                if (*_builder.transitions()[_builder.inhibitors()[i].transition].name == *oldname) {
                    const Colored::Arc &inhibArc = _builder.inhibitors()[i];
                    if (_sumPlacesNames.size() < inhibArc.place) _sumPlacesNames.resize(inhibArc.place + 1);
                    auto placeName = _sumPlacesNames[inhibArc.place];

                    if (placeName == nullptr || placeName->empty()) {
                        const PetriEngine::Colored::Place& place = _builder.places()[inhibArc.place];
                        auto sumPlaceName = std::make_shared<const_string>(*place.name + "Sum");
                        ptBuilder.addPlace(sumPlaceName, place.marking.size(), place._x + 30, place._y - 30);
                        if (_ptplacenames.count(place.name) <= 0) {
                            _ptplacenames[place.name][place.type->size()] = sumPlaceName;
                        }
                        placeName = _sumPlacesNames[inhibArc.place] = std::move(sumPlaceName);
                    }
                    ptBuilder.addInputArc(placeName, newname, true, inhibArc.inhib_weight);
                }
            }
        }

        void Unfolder::unfoldArc(PetriNetBuilder& ptBuilder, const Colored::Arc& arc, const Colored::BindingMap& binding, const shared_const_string& tName) {
            const PetriEngine::Colored::Place& place = _builder.places()[arc.place];
            //If the place is stable, the arc does not need to be unfolded.
            //This exploits the fact that since the transition is being unfolded with this binding
            //we know that this place contains the tokens to activate the transition for this binding
            //because color fixpoint allowed the binding

            assert(_partition.partition().size() > arc.place);
            const Colored::ExpressionContext context{binding, _builder.colors(), _partition.partition()[arc.place]};
            const auto ms = Colored::EvaluationVisitor::evaluate(*arc.expr, context);
            int shadowWeight = 0;
            
            const Colored::Color *newColor;
            std::vector<uint32_t> tupleIds;
            for (const auto& color : ms) {
                if (color.second == 0) {
                    continue;
                }

                if (!_partition.computed() || _partition.partition()[arc.place].isDiagonal()) {
                    newColor = color.first;
                } else {
                    tupleIds.clear();
                    color.first->getTupleId(tupleIds);

                    _partition.partition()[arc.place].applyPartition(tupleIds);
                    newColor = place.type->getColor(tupleIds);
                }

                shadowWeight += color.second;
                uint32_t id;
                if (!_partition.computed() || _partition.partition()[arc.place].isDiagonal()) {
                    id = newColor->getId();
                } else {
                    id = _partition.partition()[arc.place].getUniqueIdForColor(newColor);
                }
                auto pName = _ptplacenames[place.name][id];

                if (pName == nullptr || pName->empty()) {
                    unfoldPlace(ptBuilder, &place, newColor, arc.place, id);
                    pName = _ptplacenames[place.name][id];
                }

                if (arc.input) {
                    ptBuilder.addInputArc(pName, tName, false, color.second);
                } else {
                    ptBuilder.addOutputArc(tName, pName, color.second);
                }
                ++_nptarcs;
            }

            if (place.inhibitor) {
                if (_sumPlacesNames.size() <= arc.place) _sumPlacesNames.resize(arc.place + 1);
                auto& sumPlaceName = _sumPlacesNames[arc.place];
                if (sumPlaceName == nullptr || sumPlaceName->empty()) {
                    auto newSumPlaceName = std::make_shared<const_string>(*place.name + "Sum");
                    ptBuilder.addPlace(newSumPlaceName, place.marking.size(), place._x + 30, place._y - 30);
                    sumPlaceName = _sumPlacesNames[arc.place] = std::move(newSumPlaceName);
                }


                if (shadowWeight > 0) {
                    if (!arc.input) {
                        ptBuilder.addOutputArc(tName, sumPlaceName, shadowWeight);
                    } else {
                        ptBuilder.addInputArc(sumPlaceName, tName, false, shadowWeight);
                    }
                    ++_nptarcs;
                }
            }
        }

    
        void Unfolder::storeBinding(const shared_const_string& name, const Colored::BindingMap& binding) {
            if (_print_bindings) { 
                Colored::BindingMap b;
                for (auto var: binding) {
                    b[var.first] = var.second;
                } 
                _transitionBinding[*name] = b;
            }
        }

        void  Unfolder::printBinding(){
            if (_print_bindings) {
                std::cout << "<bindings>\n";
                for (auto const transition : _transitionBinding) {
                    std::cout << "   <transition id=\"" << transition.first << "\">\n";    
                    for(auto const var: transition.second) {
                        std::cout << "      <variable id=\"" << var.first->name << "\">\n";
                        std::cout << "         <color>" << var.second->getColorName() << "</color>\n";
                        std::cout << "      </variable>\n";
                    }
                    std::cout << "   </transition>\n";
                }
                std::cout << "</bindings>\n";
            }
        }
    }
}
