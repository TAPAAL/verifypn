//
// Created by emil on 2/19/25.
//

#ifndef COLOREDPETRINETELEMENTS_H
#define COLOREDPETRINETELEMENTS_H

namespace PetriEngine::ExplicitColored {
    struct ColorType {
        ColorType() = delete;
        explicit ColorType(const Color_t colorSize, std::vector<Color_t> basicColorSizes) :
            colorSize(colorSize), basicColorSizes(std::move(basicColorSizes)) {}
        explicit ColorType(const Color_t colorSize) :
            colorSize(colorSize), basicColorSizes(std::vector<Color_t>{colorSize}) {}
        Color_t colorSize{};
        std::vector<Color_t> basicColorSizes{};
    };
}
#endif //COLOREDPETRINETELEMENTS_H
