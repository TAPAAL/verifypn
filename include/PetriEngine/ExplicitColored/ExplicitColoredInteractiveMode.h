#ifndef COLOREDINTERACTIVEMODE_H
#define COLOREDINTERACTIVEMODE_H
#include <string>

namespace PetriEngine::ExplicitColored {
    class ExplicitColoredInteractiveMode {
    public:
        static int run(const std::string& model_path);
    };
}
#endif //COLOREDINTERACTIVEMODE_H
