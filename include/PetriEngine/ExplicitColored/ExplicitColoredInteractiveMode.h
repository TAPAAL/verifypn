#ifndef COLOREDINTERACTIVEMODE_H
#define COLOREDINTERACTIVEMODE_H
#include <optional>
#include <string>

#include "ColoredPetriNetMarking.h"
#include "ExplicitColoredPetriNetBuilder.h"

namespace PetriEngine::ExplicitColored {
    class ExplicitColoredInteractiveMode {
    public:
        static int run(const std::string& model_path);
    private:
        static std::optional<ColoredPetriNetMarking> parseMarking(const ExplicitColoredPetriNetBuilder& builder, const ColoredPetriNet& cpn, std::string markingXml, std::ostream& errorOut);
    };
}
#endif //COLOREDINTERACTIVEMODE_H
