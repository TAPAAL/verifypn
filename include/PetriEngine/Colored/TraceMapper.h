#ifndef TRACE_MAPPER_H
#define TRACE_MAPPER_H

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace PetriEngine::Colored {
    class Unfolder;

    class TraceMapper {
    public:
        using Binding = std::pair<std::string, std::string>;
        using Bindings = std::vector<Binding>;

        static TraceMapper fromUnfolder(const Unfolder& unfolder);

        std::optional<std::string_view> mapPlace(const std::string& name) const;
        std::string_view mapTransition(const std::string& name) const;
        void printTransition(std::ostream& out, const std::string& name,
                             const std::string& indent, std::optional<size_t> index = std::nullopt) const;

    private:
        std::unordered_map<std::string, std::string> _places;
        std::unordered_map<std::string, std::string> _transitions;
        std::unordered_map<std::string, Bindings> _bindings;
    };
}

#endif
