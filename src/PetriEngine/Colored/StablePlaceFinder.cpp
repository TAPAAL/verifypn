
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/StablePlaceFinder.h"

namespace PetriEngine {
    namespace Colored {

        void StablePlaceFinder::compute() {
            auto& places = _builder.places();
            auto& transitions = _builder.transitions();
            _stable.resize(places.size());
            for (uint32_t placeId = 0; placeId < places.size(); ++placeId) {
                if (places[placeId].skipped) continue;
                _stable[placeId] = true;
                if (!places[placeId]._post.empty() &&
                    places[placeId]._post.size() == places[placeId]._pre.size()) {

                    for (auto transitionId : places[placeId]._post) {
                        bool matched = false;
                        for (auto transitionId2 : places[placeId]._pre) {
                            if (transitionId == transitionId2) {
                                matched = true;
                                break;
                            }
                        }
                        if (!matched) {
                            _stable[placeId] = false;
                            break;
                        }
                        const Colored::Arc *inArc;
                        for (const auto &arc : transitions[transitionId].input_arcs) {
                            if (arc.place == placeId) {
                                inArc = &arc;
                                break;
                            }
                        }
                        bool mirroredArcs = false;
                        for (auto& arc : transitions[transitionId].output_arcs) {
                            if (arc.place == placeId) {

                                if (to_string(*arc.expr) == to_string(*inArc->expr)) {
                                    mirroredArcs = true;
                                }
                                break;
                            }
                        }
                        if (!mirroredArcs) {
                            _stable[placeId] = false;
                            break;
                        }
                    }
                } else {
                    _stable[placeId] = false;
                }
            }
        }
    }
}