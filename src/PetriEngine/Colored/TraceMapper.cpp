#include "PetriEngine/Colored/TraceMapper.h"

#include "PetriEngine/Colored/Unfolder.h"

#include <algorithm>
#include <stdexcept>

namespace PetriEngine::Colored {
    TraceMapper TraceMapper::fromUnfolder(const Unfolder& unfolder) {
        TraceMapper mapper;
        for (const auto& [original, unfolded] : unfolder.place_names()) {
            for (const auto& [_, name] : unfolded) {
                mapper._places[*name] = *original;
            }
        }

        for (const auto& [original, unfolded] : unfolder.transition_names()) {
            for (const auto& name : unfolded) {
                mapper._transitions[*name] = *original;
            }
        }

        for (const auto& [transition, binding] : unfolder.transition_bindings()) {
            auto& values = mapper._bindings[transition];
            for (const auto& [variable, color] : binding) {
                values.emplace_back(variable->name, color->getColorName());
            }

            std::sort(values.begin(), values.end());
        }

        return mapper;
    }

    std::optional<std::string_view> TraceMapper::mapPlace(const std::string& name) const {
        const auto it = _places.find(name);
        if (it == _places.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    std::string_view TraceMapper::mapTransition(const std::string& name) const {
        const auto it = _transitions.find(name);
        if (it == _transitions.end()) {
            throw std::out_of_range("Transition \"" + name + "\" not found");
        }

        return it->second;
    }

    void TraceMapper::printTransition(std::ostream& out, const std::string& name,
                                      const std::string& indent, std::optional<size_t> index) const {
        out << indent << "<transition id=\"" << mapTransition(name) << '"';
        if (index) {
            out << " index=\"" << *index << '"';
        }

        out << ">\n" << indent << "\t<bindings>\n";
        const auto it = _bindings.find(name);
        if (it != _bindings.end()) {
            for (const auto& [variable, color] : it->second) {
                out << indent << "\t\t<variable id=\"" << variable << "\"><color>"
                    << color << "</color></variable>\n";
            }
        }
        
        out << indent << "\t</bindings>\n";
    }
}
