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
        ColoredSuccessorGenerator successorGenerator(cpn);

        ExplicitColoredInteractiveMode self(successorGenerator, cpn, builder);
        return self._run_internal();
    }

    ExplicitColoredInteractiveMode::ExplicitColoredInteractiveMode(
        const ColoredSuccessorGenerator& successorGenerator,
        const ColoredPetriNet& cpn,
        const ExplicitColoredPetriNetBuilder& builder
    )
    : _successorGenerator(successorGenerator), _cpn(cpn), _builder(builder) { }

    int ExplicitColoredInteractiveMode::_run_internal() {
        auto initialMarkingInput = _readUntilDoubleNewline(std::cin);

        rapidxml::xml_document<> initialMarkingXml;
        initialMarkingXml.parse<0>(initialMarkingInput.data());

        auto initialMarkingOption = _parseMarking(initialMarkingXml, std::cerr);

        if (!initialMarkingOption.has_value()) {
            return 1;
        }

        auto marking = std::move(*initialMarkingOption);

        _printValidBindings(std::cout, marking);
        std::cout << std::endl << std::endl;

        while (true) {
            auto input = _readUntilDoubleNewline(std::cin);

            rapidxml::xml_document<> inputXml;
            inputXml.parse<0>(input.data());

            if (inputXml.first_node() == nullptr) {
                std::cerr << "No xml was given" << std::endl;
            }

            if (inputXml.first_node()->name() == std::string("marking")) {
                auto markingOption = _parseMarking(initialMarkingXml, std::cerr);

                if (!markingOption.has_value()) {
                    return 1;
                }

                marking = std::move(*markingOption);
            } else {
                auto selectedTransition = _parseTransition(inputXml, std::cerr);

                if (!selectedTransition.has_value()) {
                    return 1;
                }

                _successorGenerator.fire(marking, selectedTransition->first, selectedTransition->second);

                _printCurrentMarking(std::cout, marking);
                std::cout << std::endl << std::endl;
            }
            _printValidBindings(std::cout, marking);
            std::cout << std::endl << std::endl;
        }
        return 0;
    }

    std::string ExplicitColoredInteractiveMode::_readUntilDoubleNewline(std::istream& in) {
        in >> std::noskipws;
        std::stringstream rv;
        std::string line;
        bool lastIsNewline = false;
        while (std::getline(in, line)) {
            if (line.empty()) {
                if (lastIsNewline) {
                    return rv.str();
                }
                lastIsNewline = true;
            } else {
                lastIsNewline = false;
            }
            rv << line;
        }
        return rv.str();
    }

    void ExplicitColoredInteractiveMode::_printValidBindings(std::ostream &out, const ColoredPetriNetMarking &currentMarking) const {
        out << "<valid-bindings>" << std::endl;
        for (Transition_t transition = 0; transition < _cpn.getTransitionCount(); transition++) {
            Binding_t currentBinding = 0;
            bool first = true;
            while (true) {
                Binding actualBinding = {};
                currentBinding = _successorGenerator.findNextValidBinding(currentMarking, transition, currentBinding, _cpn.getTotalBindings(transition), actualBinding, 0);
                if (currentBinding == std::numeric_limits<Binding_t>::max()) {
                    break;
                }
                if (first) {
                    out << "\t<transition id=" << std::quoted(_builder.getTransitionName(transition)) << ">" << std::endl;
                    first = false;
                }
                out << "\t\t<binding>" << std::endl;
                for (const auto& [key, val] : actualBinding.getValues()) {
                    out << "\t\t\t<variable id=" << std::quoted(_builder.getVariableName(key)) << ">" << std::endl;
                    const auto& colorType = *_builder.getUnderlyingVariableColorTypes().at(key);

                    const auto& colorId = colorType[val];
                    out << "\t\t\t\t<color>" << colorId.getColorName() << "</color>" << std::endl;
                    out << "\t\t\t</variable>" << std::endl;
                }
                out << "\t\t</binding>" << std::endl;
                currentBinding++;
            }
            if (!first) {
                out << "\t</transition>" << std::endl;
            }
        }
        out << "</valid-bindings>" << std::endl;
    }

    std::optional<Color_t> ExplicitColoredInteractiveMode::_findColorIndex(
        const Colored::ColorType* colorType,
        const char *colorName
    ) const {
        for (const auto& color : *colorType) {
            if (color.getColorName() == colorName) {
                return color.getId();
            }
        }
        return std::nullopt;
    }

    std::optional<ColoredPetriNetMarking> ExplicitColoredInteractiveMode::_parseMarking(
        const rapidxml::xml_document<>& markingXml,
        std::ostream& errorOut
    ) const {
        const auto root = markingXml.first_node();

        ColoredPetriNetMarking generatedMarking;
        generatedMarking.markings.resize(_builder.getPlaceCount());

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

            auto markingPlaceIndexIt = _builder.getPlaceIndices().find(placeNode->first_attribute("id")->value());
            if (markingPlaceIndexIt == _builder.getPlaceIndices().end()) {
                errorOut << "Unknown place id " << std::quoted(placeNode->first_attribute("id")->value()) << std::endl;
                return std::nullopt;
            }
            auto tokenNode = placeNode->first_node();
            auto placeColorType = _builder.getPlaceUnderlyingColorType(markingPlaceIndexIt->second);
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
                const auto& colorCodec = _cpn.getPlaces()[markingPlaceIndexIt->second].colorType->colorCodec;
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

                    auto colorIndex = _findColorIndex(currentColorType, colorName);
                    if (colorIndex == std::nullopt) {
                        errorOut << "Unknown color id " << std::quoted(colorName) << std::endl;
                        return std::nullopt;
                    }
                    encodedColor = colorCodec.addToValue(encodedColor, componentIndex, *colorIndex);

                    componentIndex++;
                } while ((colorComponentNode = colorComponentNode->next_sibling()) != nullptr);
                generatedMarking.markings[markingPlaceIndexIt->second].addCount(encodedColor, count);
            } while ((tokenNode = tokenNode->next_sibling()) != nullptr);
        } while ((placeNode = placeNode->next_sibling()) != nullptr);

        return generatedMarking;
    }

    std::optional<std::pair<Transition_t, Binding>> ExplicitColoredInteractiveMode::_parseTransition(
        const rapidxml::xml_document<>& transitionXml,
        std::ostream &errorOut
    ) const {
        const auto transitionNode = transitionXml.first_node();
        if (transitionNode->name() != std::string("transition")) {
            errorOut << "Unexpected tag " << std::quoted(transitionNode->name()) << std::endl;
            return std::nullopt;
        }
        if (transitionNode->first_attribute("id") == nullptr) {
            errorOut << "Missing id attribute on transition tag" << std::endl;
            return std::nullopt;
        }
        const auto transitionId = transitionNode->first_attribute("id")->value();
        const auto transitionIndexIt = _builder.getTransitionIndices().find(transitionId);
        if (transitionIndexIt == _builder.getTransitionIndices().end()) {
            errorOut << "Unknown transition id "<< std::quoted(transitionId) << std::endl;
            return std::nullopt;
        }

        const auto bindingNode = transitionNode->first_node();
        if (bindingNode->name() != std::string("binding")) {
            errorOut << "Unexpected tag " << std::quoted(bindingNode->name()) << std::endl;
            return std::nullopt;
        }

        Binding binding;
        auto variableNode = bindingNode->first_node();
        do {
            if (variableNode->name() != std::string("variable")) {
                errorOut << "Unexpected tag " << std::quoted(variableNode->name()) << std::endl;
                return std::nullopt;
            }

            if (variableNode->first_attribute("id") == nullptr) {
                errorOut << "Missing id attribute on variable tag" << std::endl;
                return std::nullopt;
            }

            const auto variableId = variableNode->first_attribute("id")->value();
            const auto variableIndexIt = _builder.getVariableIndices()->find(variableId);
            if (variableIndexIt == _builder.getVariableIndices()->end()) {
                errorOut << "Unknown variable id " << std::quoted(variableId) << std::endl;
                return std::nullopt;
            }

            const auto colorNode = variableNode->first_node();
            if (colorNode->name() != std::string("color")) {
                errorOut << "Unexpected tag " << std::quoted(colorNode->name()) << std::endl;
                return std::nullopt;
            }

            if (colorNode->next_sibling() != nullptr) {
                errorOut << "Variables cannot be bound to product colors" << std::endl;
                return std::nullopt;
            }

            if (colorNode->type() != rapidxml::node_element) {
                errorOut << "Expected color tag to only contain color id" << std::endl;
                return std::nullopt;
            }

            const auto colorIndex = _findColorIndex(_builder.getUnderlyingVariableColorTypes()[variableIndexIt->second], colorNode->value());
            if (!colorIndex.has_value()) {
                errorOut << "Unknown color " << std::quoted(colorNode->value()) << " for variable " << std::quoted(variableId) << std::endl;
                return std::nullopt;
            }

            binding.setValue(variableIndexIt->second, *colorIndex);
        } while ((variableNode = variableNode->next_sibling()) != nullptr);

        return std::make_pair(transitionIndexIt->second, binding);
    }

    void ExplicitColoredInteractiveMode::_printCurrentMarking(
        std::ostream &out,
        const ColoredPetriNetMarking &currentMarking
    ) const {
        out << "<marking>" << std::endl;
        for (Place_t place = 0; place < currentMarking.markings.size(); place++) {
            out << "\t<place id=" << std::quoted(_builder.getPlaceName(place)) << ">" << std::endl;
            const auto& colorCodec = _cpn.getPlaces()[place].colorType->colorCodec;
            const auto& colorType = *_builder.getPlaceUnderlyingColorType(place);
            for (const auto& [encodedColor, count] : currentMarking.markings[place].counts()) {
                if (count > 0) {
                    out << "\t\t<token count=" << std::quoted(std::to_string(count)) << ">" << std::endl;
                    if (colorCodec.getColorCount() > 1) {
                        const auto productColorType = dynamic_cast<const Colored::ProductType*>(&colorType);
                        if (productColorType == nullptr) {
                            throw std::runtime_error("Color codec is inconsistent with underlying color type");
                        }
                        for (size_t colorIndex = 0; colorIndex < colorCodec.getColorCount(); colorIndex++) {
                            const Color_t color = colorCodec.decode(color, colorIndex);
                            out << "\t\t\t<color>"
                                << (*(productColorType->getNestedColorType(colorIndex)))[color].getColorName()
                                << "</color>"
                                << std::endl;
                        }
                    } else {
                        out << "\t\t\t<color>"
                            << colorType[encodedColor].getColorName()
                            << "</color>"
                            << std::endl;
                    }
                    out << "\t\t</token>" << std::endl;
                }
            }
            out << "\t</place>" << std::endl;
        }
        out << "</marking>" << std::endl;
    }
}
