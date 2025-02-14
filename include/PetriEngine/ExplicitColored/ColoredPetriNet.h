#ifndef COLOREDPETRINET_H
#define COLOREDPETRINET_H

#include <random>
#include <vector>
#include <memory>

#include "ArcCompiler.h"
#include "utils/structures/shared_string.h"
#include "AtomicTypes.h"
#include "ColoredPetriNetMarking.h"
#include "GuardCompiler.h"

namespace PetriEngine::ExplicitColored
{
    struct ColoredPetriNetTransition
    {
        std::unique_ptr<CompiledGuardExpression> guardExpression;
        std::set<Variable_t> variables;
        std::pair<std::map<Variable_t,std::vector<Color_t>>, uint64_t> validVariables;
    };

    struct BaseColorType
    {
        Color_t colors;
    };

    struct ColorType
    {
        uint32_t size;
        std::vector<std::shared_ptr<BaseColorType>> basicColorTypes;
    };

    struct ColoredPetriNetPlace
    {
        std::shared_ptr<ColorType> colorType;
    };

    struct ColoredPetriNetInhibitor
    {
        ColoredPetriNetInhibitor(const size_t from, const size_t to, const MarkingCount_t weight)
            : from(from), to(to), weight(weight){}
        uint32_t from;
        uint32_t to;
        MarkingCount_t weight;
    };

    struct ColoredPetriNetArc
    {
        uint32_t from;
        uint32_t to;
        std::shared_ptr<ColorType> colorType;
        std::unique_ptr<CompiledArcExpression> expression;
    };

    struct Variable
    {
        std::shared_ptr<BaseColorType> colorType;
    };

    class ColoredPetriNetBuilder;

    class ColoredPetriNet
    {
    public:
        ColoredPetriNet(ColoredPetriNet&&) = default;
        ColoredPetriNet& operator=(ColoredPetriNet&&) = default;
        ColoredPetriNet& operator=(const ColoredPetriNet&) = default;

        [[nodiscard]] const ColoredPetriNetMarking& initial() const {
            return _initialMarking;
        }

        [[nodiscard]] Transition_t getTransitionCount() const {
            return _transitions.size();
        }
    private:
        friend class ColoredPetriNetBuilder;
        friend class ColoredSuccessorGenerator;
        friend class ValidVariableGenerator;
        friend class FireabilityChecker;
        ColoredPetriNet() = default;
        std::vector<ColoredPetriNetTransition> _transitions;
        std::vector<ColoredPetriNetPlace> _places;
        std::vector<ColoredPetriNetArc> _arcs;
        std::vector<ColoredPetriNetInhibitor> _inhibitorArcs;
        std::vector<Variable> _variables;
        ColoredPetriNetMarking _initialMarking;
        std::vector<std::pair<uint32_t,uint32_t>> _transitionArcs; //Index is transition and pair is input/output arc beginning index in _arcs
        std::vector<uint32_t> _transitionInhibitors; //Index is transition and value is beginning index in _inhibitorArcs
    };
}
// PetriEngine

#endif // COLOREDPETRINET_H
