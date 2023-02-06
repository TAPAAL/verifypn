
#include "PetriEngine/Colored/VariableSymmetry.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/SymmetryVisitor.h"
#include "PetriEngine/Colored/VariableVisitor.h"


namespace PetriEngine {
    namespace Colored {

        void VariableSymmetry::default_init() {
            _symmetric_var_map.clear();
            auto& transitions = _builder.transitions();
            _symmetric_var_map.resize(transitions.size());
        }

        void VariableSymmetry::compute() {
            default_init();
            if (_builder.isColored()) {
                auto& transitions = _builder.transitions();
                for (uint32_t transitionId = 0; transitionId < transitions.size(); transitionId++) {
                    const Colored::Transition &transition = transitions[transitionId];
                    if (transition.skipped) continue;
                    if (transition.guard) {
                        continue;
                        //the variables cannot appear on the guard
                    }

                    for (const auto &inArc : transition.input_arcs) {
                        std::set<const Colored::Variable*> inArcVars;

                        //Application of symmetric variables for partitioned places is currently unhandled
                        if (_partition.computed() && !_partition.partition()[inArc.place].isDiagonal()) {
                            continue;
                        }

                        //the expressions is eligible if it is an addexpression that contains only
                        //numberOfExpressions with the same number
                        auto [isEligible, numbers] = Colored::SymmetryVisitor::eligible_for_symmetry(*inArc.expr);

                        if (isEligible && numbers.size() > 1) {
                            Colored::VariableVisitor::get_variables(*inArc.expr, inArcVars);
                            //It cannot be symmetric with anything if there is only one variable
                            if (inArcVars.size() < 2) {
                                continue;
                            }
                            //The variables may appear only on one input arc and one output arc
                            checkSymmetricVarsInArcs(transition, inArc, inArcVars, isEligible);


                            //All the variables have to appear on exactly one output arc and nowhere else
                            checkSymmetricVarsOutArcs(transition, inArcVars, isEligible);
                        } else {
                            isEligible = false;
                        }
                        if (isEligible) {
                            _symmetric_var_map[transitionId].emplace_back(inArcVars);
                        }
                    }
                }
            }
        }

        void VariableSymmetry::checkSymmetricVarsInArcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const {
            for (auto& otherInArc : transition.input_arcs) {
                if (inArc.place == otherInArc.place) {
                    continue;
                }
                std::set<const Colored::Variable*> otherArcVars;
                Colored::VariableVisitor::get_variables(*otherInArc.expr, otherArcVars);
                for (auto* var : inArcVars) {
                    if (otherArcVars.find(var) != otherArcVars.end()) {
                        isEligible = false;
                        break;
                    }
                }
            }
        }

        void VariableSymmetry::checkSymmetricVarsOutArcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const {
            uint32_t numArcs = 0;
            bool foundSomeVars = false;
            for (auto& outputArc : transition.output_arcs) {
                bool foundArc = true;
                std::set<const Colored::Variable*> otherArcVars;
                Colored::VariableVisitor::get_variables(*outputArc.expr, otherArcVars);
                for (auto* var : inArcVars) {
                    if (otherArcVars.find(var) == otherArcVars.end()) {
                        foundArc = false;
                    } else {
                        foundSomeVars = true;
                    }
                }
                if (foundArc) {
                    //Application of symmetric variables for partitioned places is currently unhandled
                    if (_partition.computed() && !_partition.partition()[outputArc.place].isDiagonal()) {
                        isEligible = false;
                        break;
                    }
                    numArcs++;
                    //All vars were present
                    foundSomeVars = false;
                }
                //If some vars are present the vars are not eligible
                if (foundSomeVars) {
                    isEligible = false;
                    break;
                }
            }

            if (numArcs != 1) {
                isEligible = false;
            }
        }

        void VariableSymmetry::printSymmetricVariables() const {
            for (uint32_t transitionId = 0; transitionId < _builder.transitions().size(); transitionId++) {
                const auto &transition = _builder.transitions()[transitionId];
                if (_symmetric_var_map[transitionId].empty()) {
                    std::cout << "Transition " << transition.name << " has no symmetric variables" << std::endl;
                } else {
                    std::cout << "Transition " << transition.name << " has symmetric variables: " << std::endl;
                    for (const auto &set : _symmetric_var_map[transitionId]) {
                        std::string toPrint = "SET: ";
                        for (auto* variable : set) {
                            toPrint += variable->name + ", ";
                        }
                        std::cout << toPrint << std::endl;
                    }
                }
            }
        }
    }
}
