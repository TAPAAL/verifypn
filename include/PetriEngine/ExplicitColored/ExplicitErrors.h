#ifndef PTRIE_TOO_SMALL_H
#define PTRIE_TOO_SMALL_H

#include <exception>

namespace PetriEngine::ExplicitColored {
    enum class ExplicitErrorType {
        PTRIE_TOO_SMALL = 0,
        UNSUPPORTED_QUERY = 1,
        UNSUPPORTED_STRATEGY = 2,
        UNSUPPORTED_GENERATOR = 3,
        UNSUPPORTED_NET = 4,
        UNEXPECTED_EXPRESSION = 5,
        UNKNOWN_VARIABLE = 6,
        TOO_MANY_TOKENS = 7,
        TOO_MANY_BINDINGS = 8,
        UNKNOWN_ENCODING = 9,
        INVALID_TRACE = 10
    };

    class explicit_error final : public std::exception {
    public:
        explicit explicit_error(const ExplicitErrorType type) : std::exception(), type(type) {
        }

        ExplicitErrorType type;

        void print(std::ostream& os) const {
            switch (type) {
                case ExplicitErrorType::UNSUPPORTED_STRATEGY:
                    os << "Strategy is not supported for explicit colored engine" << std::endl
                        << "UNSUPPORTED STRATEGY" << std::endl;
                    break;
                case ExplicitErrorType::UNSUPPORTED_QUERY:
                    os << "Query is not supported for explicit colored engine" << std::endl
                        << "UNSUPPORTED QUERY" << std::endl;
                    break;
                case ExplicitErrorType::PTRIE_TOO_SMALL:
                    os << "Marking was too big to be stored in passed list" << std::endl
                        << "PTRIE TOO SMALL" << std::endl;
                    break;
                case ExplicitErrorType::UNSUPPORTED_GENERATOR:
                    os << "Type of successor generator not supported" << std::endl
                        << "UNSUPPORTED GENERATOR" << std::endl;
                    break;
                case ExplicitErrorType::UNSUPPORTED_NET:
                    os << "Net is not supported" << std::endl
                        << "UNSUPPORTED NET" << std::endl;
                    break;
                case ExplicitErrorType::UNEXPECTED_EXPRESSION:
                    os << "Unexpected expression in arc" << std::endl
                    << "UNEXPECTED EXPRESSION" << std::endl;
                    break;
                case ExplicitErrorType::UNKNOWN_VARIABLE:
                    os << "Unknown variable in arc" << std::endl
                        << "UNKNOWN VARIABLE" << std::endl;
                    break;
                case ExplicitErrorType::TOO_MANY_TOKENS:
                    os << "Too many tokens to represent" << std::endl
                        << "TOO MANY TOKENS" << std::endl;
                    break;
                case ExplicitErrorType::TOO_MANY_BINDINGS:
                    os << "The colored petri net has too many bindings to be represented" << std::endl
                        << "TOO_MANY_BINDINGS" << std::endl;
                    break;
                case ExplicitErrorType::INVALID_TRACE:
                    os << "Trace contained unknown transition, variable or color" << std::endl
                        << "INVALID TRACE" << std::endl;
                    break;
                default:
                    os << "Something went wrong in explicit colored exploration" << std::endl
                        << "UNKNOWN EXPLICIT COLORED ERROR" << std::endl;
                    break;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const explicit_error& error) {
            error.print(os);
            return os;
        }
    };
}

#endif //PTRIE_TOO_SMALL_H
