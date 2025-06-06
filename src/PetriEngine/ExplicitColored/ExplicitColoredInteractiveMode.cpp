#include <rapidxml.hpp>
#include <iostream>
#include <iterator>
#include <vector>

#include "PetriEngine/ExplicitColored/ExplicitColoredInteractiveMode.h"

#include <iomanip>
#include <PetriEngine/ExplicitColored/ExplicitColoredPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/SuccessorGenerator/ColoredSuccessorGenerator.h>

namespace PetriEngine::ExplicitColored {
    int ExplicitColoredInteractiveMode::run(const std::string &model_path) {
        ExplicitColoredPetriNetBuilder builder;
        builder.parse_model(model_path);

        const auto status = builder.build();
        if (status == ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS) {
            throw explicit_error(ExplicitErrorType::TOO_MANY_BINDINGS);
        }

        if (status != ColoredPetriNetBuilderStatus::OK) {
            throw explicit_error(ExplicitErrorType::UNSUPPORTED_NET);
        }

        auto cpn = builder.takeNet();

        std::cin >> std::noskipws;

        std::istream_iterator<char> it(std::cin);
        std::istream_iterator<char> end;
        std::string markingXml(it, end);

        const auto markingOption = parseMarking(builder, cpn, markingXml, std::cerr);
        if (!markingOption.has_value()) {
            return 1;
        }
        const auto& marking = markingOption.value();

        ColoredSuccessorGenerator successorGenerator(cpn);
        std::cout << "<valid-bindings>" << std::endl;
        for (Transition_t transition = 0; transition < cpn.getTransitionCount(); transition++) {
            Binding_t currentBinding = 0;
            bool first = true;
            while (true) {
                Binding actualBinding = {};
                currentBinding = successorGenerator.findNextValidBinding(marking, transition, currentBinding, cpn.getTotalBindings(transition), actualBinding, 0);
                if (currentBinding == std::numeric_limits<Binding_t>::max()) {
                    break;
                }
                if (first) {
                    std::cout << "\t<transition id=" << std::quoted(builder.getPlaceName(transition)) << ">" << std::endl;
                    first = false;
                }
                std::cout << "\t\t<binding>" << std::endl;
                for (const auto& [key, val] : actualBinding.getValues()) {
                    std::cout << "\t\t\t<variable id=" << std::quoted(builder.getVariableName(key)) << ">" << std::endl;
                    const auto& colorType = *builder.getUnderlyingVariableColorTypes().at(key);

                    const auto& colorId = colorType[val];
                    std::cout << "\t\t\t\t<color id=" << std::quoted(colorId.getColorName()) << "/>" << std::endl;
                    std::cout << "\t\t\t</variable>" << std::endl;
                }
                std::cout << "\t\t</binding>" << std::endl;
                currentBinding++;
            }
            if (!first) {
                std::cout << "\t</transition>" << std::endl;
            }
        }
        std::cout << "</valid-bindings>" << std::endl;

        return 0;
    }

    std::optional<ColoredPetriNetMarking> ExplicitColoredInteractiveMode::parseMarking(const ExplicitColoredPetriNetBuilder& builder, const ColoredPetriNet& cpn, std::string markingXml, std::ostream& errorOut) {
        rapidxml::xml_document<> doc;
        doc.parse<0>(markingXml.data());
        const auto root = doc.first_node();

        ColoredPetriNetMarking generatedMarking;
        generatedMarking.markings.resize(builder.getPlaceCount());

        auto placeNode = root->first_node();
        if (root->name() != std::string("marking")) {
            errorOut << "Unexpected tag " << std::quoted(root->name()) << std::endl;
            return std::nullopt;
        }
        do {
            if (placeNode->name() != std::string("place")) {
                errorOut << "Unexpected tag " << std::quoted(placeNode->name()) << std::endl;
                return std::nullopt;
            }

            if (placeNode->first_attribute("id") == nullptr) {
                errorOut << "Place tag is missing id attribute" << std::endl;
                return std::nullopt;
            }

            auto markingPlaceIndexIt = builder.getPlaceIndices().find(placeNode->first_attribute("id")->value());
            if (markingPlaceIndexIt == builder.getPlaceIndices().end()) {
                errorOut << "Unknown place id " << std::quoted(placeNode->first_attribute("id")->value()) << std::endl;
                return std::nullopt;
            }
            auto tokenNode = placeNode->first_node();
            auto placeColorType = builder.getPlaceUnderlyingColorType(markingPlaceIndexIt->second);
            do {
                if (tokenNode->name() != std::string("token")) {
                    errorOut << "Unexpected tag " << std::quoted(tokenNode->name()) << std::endl;
                    return std::nullopt;
                }
                if (tokenNode->first_attribute("count") == nullptr) {
                    errorOut << "Token tag is missing count attribute" << std::endl;
                    return std::nullopt;
                }
                auto count = 0;
                try {
                    count = std::stoi(tokenNode->first_attribute("count")->value());
                } catch (const std::out_of_range&) {
                    errorOut << "Token count " << tokenNode->first_attribute("count")->value() << " is too big or too small" << std::endl;
                    return std::nullopt;
                } catch (const std::invalid_argument&) {
                    errorOut << "Token count " << tokenNode->first_attribute("count")->value() << " could not be parsed" << std::endl;
                    return std::nullopt;
                }
                auto colorComponentNode = tokenNode->first_node();
                uint64_t encodedColor = 0;
                const auto& colorCodec = cpn.getPlaces()[markingPlaceIndexIt->second].colorType->colorCodec;
                auto componentIndex = 0;
                do {
                    if (colorComponentNode->name() != std::string("color")) {
                        errorOut << "Unexpected tag " << colorComponentNode->name() << std::endl;
                        return std::nullopt;
                    }
                    if (colorComponentNode->type() != rapidxml::node_element) {
                        errorOut << "Expected color tag to only contain color id" << std::endl;
                        return std::nullopt;
                    }
                    const auto colorName = colorComponentNode->value();
                    auto currentColorType = placeColorType;
                    if (auto productColor = dynamic_cast<const Colored::ProductType*>(placeColorType)) {
                        currentColorType = productColor->getNestedColorType(componentIndex);
                    }
                    bool found = false;
                    for (const auto& color : *currentColorType) {
                        if (color.getColorName() == colorName) {
                            encodedColor = colorCodec.addToValue(encodedColor, componentIndex, color.getId());
                            found = true;
                            break;
                        }
                    }
                    if (found == false) {
                        errorOut << "Unknown color id " << std::quoted(colorName) << std::endl;
                        return std::nullopt;
                    }
                    componentIndex++;
                } while ((colorComponentNode = colorComponentNode->next_sibling()) != nullptr);
                generatedMarking.markings[markingPlaceIndexIt->second].addCount(encodedColor, count);
            } while ((tokenNode = tokenNode->next_sibling()) != nullptr);
        } while ((placeNode = placeNode->next_sibling()) != nullptr);
    }
}
