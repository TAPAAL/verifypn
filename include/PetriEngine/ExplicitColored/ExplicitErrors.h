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
        too_many_bindings = 8,
        unknown_encoding = 9,
    };

    class explicit_error : public std::exception {
    public:
        explicit explicit_error(ExplicitErrorType type) : std::exception(), type(type) {}
        ExplicitErrorType type;

        void print(std::ostream& os) const {
            switch (type){
                case unsupported_strategy:
                    os << "Strategy is not supported for explicit colored engine" << std::endl
                    << "UNSUPPORTED STRATEGY" << std::endl;
                    break;
                case unsupported_query:
                    os << "Query is not supported for explicit colored engine" << std::endl
                    << "UNSUPPORTED QUERY" << std::endl;
                    break;
                case ptrie_too_small:
                    os << "Marking was too big to be stored in passed list" << std::endl
                    << "PTRIE TOO SMALL" << std::endl;
                    break;
                case unsupported_generator:
                    os << "Type of successor generator not supported" << std::endl
                    << "UNSUPPORTED GENERATOR" << std::endl;
                    break;
                case unsupported_net:
                    os << "Net is not supported" << std::endl
                    << "UNSUPPORTED NET" << std::endl;
                    break;
                case unexpected_expression:
                    os << "Unexpected expression in arc" << std::endl
                    << "UNEXPECTED EXPRESSION" << std::endl;
                    break;
                case unknown_variable:
                    os << "Unknown variable in arc" << std::endl
                    << "UNKNOWN VARIABLE" << std::endl;
                    break;
                case too_many_tokens:
                    os << "Too many tokens to represent" << std::endl
                    << "TOO MANY TOKENS" << std::endl;
                    break;
                case too_many_bindings:
                    os << "The colored petri net has too many bindings to be represented" << std::endl
                            << "TOO_MANY_BINDINGS" << std::endl;
                    break;
                default:
                    os << "Something went wrong in explicit colored exploration" << std::endl
                    << "UNKNOWN EXPLICIT COLORED ERROR" << std::endl;
                    break;
                }
        }
        friend std::ostream &operator<<(std::ostream &os, const explicit_error &error) {
            error.print(os);
            return os;
        }
    };
}

#endif //PTRIE_TOO_SMALL_H