//
// Created by emil on 2/19/25.
//

#ifndef COLOREDPETRINETELEMENTS_H
#define COLOREDPETRINETELEMENTS_H

namespace PetriEngine::ExplicitColored {
//    struct BaseColorType
//    {
//        Color_t colors;
//    };

    struct ColorType {
        ColorType() = delete;
        explicit ColorType(const uint32_t colorSize, std::vector<Color_t> basicColorSizes) :
            colorSize(colorSize), basicColorSizes(std::move(basicColorSizes)) {}
        explicit ColorType(const uint32_t colorSize) :
            colorSize(colorSize), basicColorSizes(std::vector<Color_t>{colorSize}) {}
        uint32_t colorSize{};
//        std::vector<std::shared_ptr<BaseColorType>> basicColorTypes;
        std::vector<Color_t> basicColorSizes{};
    };
}
#endif //COLOREDPETRINETELEMENTS_H
