#ifndef COLOREDINTERACTIVEMODE_H
#define COLOREDINTERACTIVEMODE_H
#include <optional>
#include <string>

#include "ColoredPetriNetMarking.h"
#include "ExplicitColoredPetriNetBuilder.h"
#include "SuccessorGenerator/ColoredSuccessorGenerator.h"

namespace PetriEngine::ExplicitColored {
    class ExplicitColoredInteractiveMode {
    public:
        static int run(const std::string& model_path);
    private:
        ExplicitColoredInteractiveMode(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNet& cpn, const ExplicitColoredPetriNetBuilder& builder);
        int _run_internal();
        static std::string _readUntilDoubleNewline(std::istream& in);
        std::optional<ColoredPetriNetMarking> _parseMarking(const rapidxml::xml_document<>& markingXml, std::ostream& errorOut) const;
        std::optional<std::pair<Transition_t, Binding>> _parseTransition(const rapidxml::xml_document<>& transitionXml, std::ostream& errorOut) const;
        void _printCurrentMarking(std::ostream& out, const ColoredPetriNetMarking& currentMarking) const;
        void _printValidBindings(std::ostream& out, const ColoredPetriNetMarking& currentMarking) const;
        std::optional<Color_t> _findColorIndex(const Colored::ColorType* colorType, const char* colorName) const;
        const ColoredSuccessorGenerator& _successorGenerator;
        const ColoredPetriNet& _cpn;
        const ExplicitColoredPetriNetBuilder& _builder;
    };
}
#endif //COLOREDINTERACTIVEMODE_H
