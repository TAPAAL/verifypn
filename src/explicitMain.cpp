#include "explicitMain.h"

#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include "PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h"
#include "PetriEngine/Colored/PnmlWriter.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h"
#include "VerifyPN.h"
#include "PetriEngine/options.h"
#include "utils/NullStream.h"
#include "PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;
using namespace PetriEngine::ExplicitColored;

int explicitColored(options_t& options, shared_string_set& string_set, std::vector<Condition_ptr>& queries, const std::vector<std::string>& queryNames) {
    ColorIgnorantPetriNetBuilder ignorantBuilder(string_set);

    ignorantBuilder.parse_model(options.modelfile);
    ignorantBuilder.sort();

    auto status = ignorantBuilder.build();
    if (status == ExplicitColored::ColoredIgnorantPetriNetBuilderStatus::CONTAINS_NEGATIVE) {
        std::cout << "Cannot do color ignorant check" << std::endl;
    } else {
        auto net = ignorantBuilder.getPetriNet();
        std::unique_ptr<MarkVal[]> qm0(net->makeInitialMarking());
        simplify_queries(qm0.get(), net.get(), queries, options, std::cout);
    }

    std::cout << "Using explicit colored" << std::endl;
    NullStream nullStream;
    std::ostream &fullStatisticOut = options.printstatistics == StatisticsLevel::Full ? std::cout : nullStream;
    ExplicitColored::ColoredPetriNetBuilder builder;
    if (options.enablecolreduction) {
        PetriEngine::ColoredPetriNetBuilder cpnBuilder(string_set);
        cpnBuilder.parse_model(options.modelfile);
        bool result = reduceColored(cpnBuilder, queries, options.logic, options.colReductionTimeout, fullStatisticOut,
                      options.enablecolreduction, options.colreductions);
        if (!result) {
            std::cout << "Could not do colored reductions" << std::endl;
            builder.parse_model(options.modelfile);
        } else {
            std::stringstream cpnOut;
            Colored::PnmlWriter writer(cpnBuilder, cpnOut);
            writer.toColPNML();
            builder.parse_model(cpnOut);
            fullStatisticOut << std::endl;
        }
    } else {
        builder.parse_model(options.modelfile);
    }
    auto buildStatus = builder.build();

    switch (buildStatus) {
    case ColoredPetriNetBuilderStatus::OK:
        break;
    case ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS:
        std::cout << "The colored petri net has too many bindings to be represented" << std::endl
                << "TOO_MANY_BINDINGS" << std::endl;
        return to_underlying(ReturnValue::UnknownCode);
    default:
        std::cout << "The explicit colored petri net builder gave an unexpected error " << static_cast<int>(
                    buildStatus) << std::endl
                << "Unknown builder error " << static_cast<int>(buildStatus) << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    }

    auto net = builder.takeNet();
    bool result = false;
    auto placeIndices = builder.takePlaceIndices();
    auto transitionIndices = builder.takeTransitionIndices();

    for (size_t i = 0; i < queries.size(); i++) {
        const auto seed = options.seed();
        ColoredResultPrinter resultPrinter(i, fullStatisticOut, queryNames, seed);
        ExplicitWorklist worklist(net, queries[i], placeIndices, transitionIndices, resultPrinter, seed);
        switch (options.strategy) {
        case Strategy::DEFAULT:
        case Strategy::DFS:
            result = worklist.check(SearchStrategy::DFS, options.colored_sucessor_generator);
            break;
        case Strategy::BFS:
            result = worklist.check(SearchStrategy::BFS, options.colored_sucessor_generator);
            break;
        case Strategy::RDFS:
            result = worklist.check(SearchStrategy::RDFS, options.colored_sucessor_generator);
            break;
        case Strategy::HEUR:
            result = worklist.check(SearchStrategy::HEUR, options.colored_sucessor_generator);
            break;
        default:
            throw explicit_error{ExplicitErrorType::unsupported_strategy};
        }
    }
    if (result) {
        return to_underlying(ReturnValue::SuccessCode);
    }
    return to_underlying(ReturnValue::FailedCode);
}

int explicitColoredErrorHandler(const ExplicitColored::explicit_error& error) {
    switch (error.type){
    case ExplicitErrorType::unsupported_strategy:
        std::cout << "Strategy is not supported for explicit colored engine" << std::endl
        << "UNSUPPORTED STRATEGY" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::unsupported_query:
        std::cout << "Query is not supported for explicit colored engine" << std::endl
        << "UNSUPPORTED QUERY" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::ptrie_too_small:
        std::cout << "Marking was too big to be stored in passed list" << std::endl
        << "PTRIE TOO SMALL" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::unsupported_generator:
        std::cout << "Type of successor generator not supported" << std::endl
        << "UNSUPPORTED GENERATOR" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::unsupported_net:
        std::cout << "Net is not supported" << std::endl
        << "UNSUPPORTED NET" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::unexpected_expression:
        std::cout << "Unexpected expression in arc" << std::endl
        << "UNEXPECTED EXPRESSION" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::unknown_variable:
        std::cout << "Unknown variable in arc" << std::endl
        << "UNKNOWN VARIABLE" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    case ExplicitErrorType::too_many_tokens:
        std::cout << "Too many tokens to represent" << std::endl
        << "TOO MANY TOKENS" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    default:
        std::cout << "Something went wrong in explicit colored exploration" << std::endl
        << "UNKNOWN EXPLICIT COLORED ERROR" << std::endl;
        return to_underlying(ReturnValue::ErrorCode);
    }
}