//
// Created by emil on 2/12/25.
//

#ifndef PTRIE_TOO_SMALL_H
#define PTRIE_TOO_SMALL_H

#include <exception>


namespace PetriEngine::ExplicitColored {
    enum ExplicitErrorType {
        ptrie_too_small = 0,
        unsupported_query = 1,
        unsupported_strategy = 2,
        unsupported_generator = 3,
        unsupported_net = 4,
        unexpected_expression = 5,
        unknown_variable = 6,
        too_many_tokens = 7,
    };

    class explicit_error : public std::exception {
    public:
        explicit explicit_error(ExplicitErrorType type) : std::exception(), type(type) {}
        ExplicitErrorType type;
    };
}

#endif //PTRIE_TOO_SMALL_H