/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_COLOREDREDUCER_H
#define VERIFYPN_COLOREDREDUCER_H

#include <utility>

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/PQL/PlaceUseVisitor.h"
#include "ReductionRule.h"
#include "RedRuleIdentity.h"


namespace PetriEngine::Colored {
    using CArcIter = std::vector<Arc>::iterator;

    namespace Reduction {

        struct ApplicationSummary {
            std::string name;
            uint32_t applications;

            ApplicationSummary(std::string name, uint32_t applications) : name(std::move(name)),
                                                                          applications(applications) {}

            bool operator<(const ApplicationSummary &rhs) const { return name < rhs.name; }
        };

        class ColoredReducer {
        public:
            ColoredReducer(PetriEngine::ColoredPetriNetBuilder &b) : _builder(b),
                                                                     _origPlaceCount(b.getPlaceCount()),
                                                                     _origTransitionCount(
                                                                             b.getTransitionCount()) {
#ifndef NDEBUG
                // All rule names must be unique
                std::set<std::string> names;
                for (auto &rule : _reductions) {
                    assert(names.find(rule->name()) == names.end());
                    names.insert(rule->name());
                }
#endif
            }

            std::vector<ApplicationSummary> createApplicationSummary() const;

            bool reduce(uint32_t timeout, const std::vector<bool> &inQuery, bool preserveDeadlocks);

            double time() const {
                return _timeSpent;
            }

            bool hasTimedOut() const {
                auto now = std::chrono::high_resolution_clock::now();
                return std::chrono::duration_cast<std::chrono::seconds>(now - _startTime).count() >= _timeout;
            }

            uint32_t origPlaceCount() const {
                return _origPlaceCount;
            }

            uint32_t origTransitionCount() const {
                return _origTransitionCount;
            }

            uint32_t unskippedPlacesCount() const {
                return _builder.getPlaceCount() - _skippedPlaces.size();
            }

            uint32_t unskippedTransitionsCount() {
                return _builder.getTransitionCount() - _skippedTransitions.size();
            }

            const std::vector<Colored::Place> &places() const {
                return _builder.places();
            }

            const std::vector<Colored::Transition> &transitions() const {
                return _builder.transitions();
            }

            const std::vector<Colored::Arc> &inhibitorArcs() const {
                return _builder.inhibitors();
            }

            CArcIter getInArc(uint32_t pid, Colored::Transition &tran) const;

            CArcIter getOutArc(Colored::Transition &tran, uint32_t pid) const;

            void skipPlace(uint32_t pid);

            void skipTransition(uint32_t tid);


        private:
            PetriEngine::ColoredPetriNetBuilder &_builder;
            std::chrono::system_clock::time_point _startTime;
            uint32_t _timeout = 0;
            double _timeSpent = 0;
            uint32_t _origPlaceCount;
            uint32_t _origTransitionCount;
            std::vector<uint32_t> _skippedPlaces;
            std::vector<uint32_t> _skippedTransitions;

            // Reduction rules
            RedRuleIdentity _reduceFirstPlace;
            std::vector<ReductionRule *> _reductions{
                    // TODO Actually useful reductions. This is just a test rule to guide implementation
                    &_reduceFirstPlace
            };
        };
    }
}

#endif //VERIFYPN_COLOREDREDUCER_H
