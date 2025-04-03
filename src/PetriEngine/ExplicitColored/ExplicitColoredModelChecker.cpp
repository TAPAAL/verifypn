#include "PetriEngine/ExplicitColored/ExplicitColoredModelChecker.h"

#include <VerifyPN.h>
#include <PetriEngine/Colored/PnmlWriter.h>
#include <PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h>
#include <PetriEngine/PQL/Evaluation.h>
#include <utils/NullStream.h>
#include <sstream>

namespace PetriEngine::ExplicitColored {
    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkQuery(
        const std::string& modelPath,
        const Condition_ptr& query,
        options_t& options,
        IColoredResultPrinter* resultPrinter
    ) const {
        std::stringstream pnmlModelStream;
        Result result = Result::UNKNOWN;
        std::ifstream modelFile(modelPath);
        pnmlModelStream << modelFile.rdbuf();
        std::string pnmlModel = std::move(pnmlModelStream).str();

        if (options.enablecolreduction) {
            std::stringstream reducedPnml;
            _reduce(pnmlModel, reducedPnml, query, options);
            pnmlModel = std::move(reducedPnml).str();
        }

        if (options.use_query_reductions) {
            result = checkColorIgnorantLP(pnmlModel, query, options);
            if (result != Result::UNKNOWN) {
                if (resultPrinter) {
                    //TODO: add other techniques
                    resultPrinter->printNonExplicitResult(
                        {"COLOR_IGNORANT", "QUERY_REDUCTION", "SAT_SMT", "LP_APPROX" },
                        result == Result::SATISFIED
                            ? AbstractHandler::Result::Satisfied
                            : AbstractHandler::Result::NotSatisfied
                    );
                }
                return result;
            }
        }

        SearchStatistics searchStatistics;
        auto [newResult, trace] = explicitColorCheck(pnmlModel, query, options, &searchStatistics);
        result = newResult;
        if (result != Result::UNKNOWN) {
            if (resultPrinter) {
                resultPrinter->printResult(
                    searchStatistics,
                    result == Result::SATISFIED
                        ? AbstractHandler::Result::Satisfied
                        : AbstractHandler::Result::NotSatisfied,
                    trace.has_value()
                        ? &trace.value()
                        : nullptr
                );
            }
            return result;
        }
        return Result::UNKNOWN;
    }

    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkColorIgnorantLP(
        const std::string& pnmlModel,
        const Condition_ptr& query,
        options_t& options
    ) const {
        if (!isReachability(query)) {
            return Result::UNKNOWN;
        }
        ContainsFireabilityVisitor has_fireability;
        Visitor::visit(has_fireability, query);
        if (has_fireability.getReturnValue()) {
            return Result::UNKNOWN;
        }
        auto queryCopy = ConditionCopyVisitor::copyCondition(query);

        bool isEf = false;
        if (dynamic_cast<EFCondition*>(queryCopy.get())) {
            isEf = true;
        }
        ColorIgnorantPetriNetBuilder ignorantBuilder(_stringSet);
        std::stringstream pnmlModelStream {pnmlModel};
        ignorantBuilder.parse_model(pnmlModelStream);
        auto status = ignorantBuilder.build();
        if (status == ColoredIgnorantPetriNetBuilderStatus::CONTAINS_NEGATIVE) {
            return Result::UNKNOWN;
        }

        auto builder = ignorantBuilder.getUnderlying();
        auto qnet = std::unique_ptr<PetriNet>(builder.makePetriNet(false));
        std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
        
        std::vector queries { std::move(queryCopy) };

        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);

        {
            EvaluationContext context(qm0.get(), qnet.get());

            auto r = evaluate(queries[0].get(), context);
            if(r == Condition::RFALSE)
            {
                queries[0] = BooleanCondition::FALSE_CONSTANT;
            }
            else if(r == Condition::RTRUE)
            {
                queries[0] = BooleanCondition::TRUE_CONSTANT;
            }
        }

        // simplification. We always want to do negation-push and initial marking check.
        simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);

        if (queries[0] == BooleanCondition::FALSE_CONSTANT && isEf) {
            return Result::UNSATISFIED;
        }
        if (queries[0] == BooleanCondition::TRUE_CONSTANT && !isEf) {
            return Result::SATISFIED;
        }
        return Result::UNKNOWN;
    }

    std::pair<ExplicitColoredModelChecker::Result, std::optional<std::vector<TraceStep>>> ExplicitColoredModelChecker::explicitColorCheck(
        const std::string& pnmlModel,
        const Condition_ptr& query,
        options_t& options,
        SearchStatistics* searchStatistics
    ) const {
        ColoredPetriNetBuilder cpnBuilder;
        auto pnmlModelStream = std::istringstream {pnmlModel};
        cpnBuilder.parse_model(pnmlModelStream);

        switch (const auto buildStatus = cpnBuilder.build()) {
            case ColoredPetriNetBuilderStatus::OK:
                break;
            case ColoredPetriNetBuilderStatus::TOO_MANY_BINDINGS:
                std::cout << "The colored petri net has too many bindings to be represented" << std::endl
                        << "TOO_MANY_BINDINGS" << std::endl;
                return std::make_pair(Result::UNKNOWN, std::nullopt);
            default:
                throw base_error("Unknown builder error ", static_cast<uint32_t>(buildStatus));
        }

        auto net = cpnBuilder.takeNet();

        ExplicitWorklist worklist(net, query, cpnBuilder.getPlaceIndices(), cpnBuilder.getTransitionIndices(), options.seed(), options.trace != TraceLevel::None);
        bool result = worklist.check(options.strategy, options.colored_sucessor_generator);

        if (searchStatistics) {
            *searchStatistics = worklist.GetSearchStatistics();
        }

        std::optional<std::vector<TraceStep>> trace = std::nullopt;
        if (options.trace != TraceLevel::None) {
            auto counterExample = worklist.getCounterExampleId();

            if (counterExample.has_value()) {
                auto internalTrace  = worklist.getTraceTo(counterExample.value());
                if (internalTrace.has_value()) {
                    trace = _translateTraceStep(internalTrace.value(), cpnBuilder, net);
                }
            }
        }
        return std::make_pair(result ? Result::SATISFIED : Result::UNSATISFIED, trace);
    }

    void ExplicitColoredModelChecker::_reduce(
        const std::string& pnmlModel,
        std::stringstream& out,
        const Condition_ptr& query,
        options_t& options
    ) const {
        PetriEngine::ColoredPetriNetBuilder cpnBuilder(_stringSet);
        std::stringstream pnmlModelStream {pnmlModel};
        cpnBuilder.parse_model(pnmlModelStream);
        auto queries = std::vector{query};
        const bool result = reduceColored(cpnBuilder, queries, options.logic, options.colReductionTimeout, _fullStatisticOut,
                      options.enablecolreduction, options.colreductions);
        std::stringstream cpnResult;
        if (!result) {
            std::cout << "Could not do colored reductions" << std::endl;
            std::stringstream cpnOut;
            out << pnmlModel;
            return;
        }
        Colored::PnmlWriter writer(cpnBuilder, out);
        writer.toColPNML();
        _fullStatisticOut << std::endl;
        out << cpnResult.rdbuf();
    }

    std::vector<TraceStep> ExplicitColoredModelChecker::_translateTraceStep(
        const std::vector<InternalTraceStep> &internalTrace,
        const ColoredPetriNetBuilder& cpnBuilder,
        const ColoredPetriNet& net
    ) const {
        std::unordered_map<Transition_t, std::string> transitionToId;
        for (const auto& [key, value] : cpnBuilder.getTransitionIndices()) {
            transitionToId.emplace(value, key);
        }

        std::unordered_map<Variable_t, std::string> variableToId;
        for (const auto& [key, value] : *cpnBuilder.getVariableIndices()) {
            variableToId.emplace(value, key);
        }

        std::unordered_map<Place_t, std::string> placeToId;
        for (const auto& [key, value] : cpnBuilder.getPlaceIndices()) {
            placeToId.emplace(value, key);
        }

        const auto& variableColorTypes = cpnBuilder.getUnderlyingVariableColorTypes();

        ColoredSuccessorGenerator successorGenerator(net);
        auto currentState = net.initial();
        std::vector<TraceStep> trace;
        for (auto& traceStep : internalTrace) {
            std::unordered_map<std::string, std::string> binding;
            for (auto [variable, value] : traceStep.binding.getValues()) {
                const auto variableIt = variableToId.find(variable);
                if (variableIt == variableToId.end()) {
                    std::cerr << "Could not find variable id corresponding to variable index " << variable << std::endl;
                    throw explicit_error(ExplicitErrorType::invalid_trace);
                }
                if (variable >= variableColorTypes.size()) {
                    std::cerr << "Could not find color type for variable index " << variable << "(" << variableIt->second << ")" << std::endl;
                    throw explicit_error(ExplicitErrorType::invalid_trace);
                }
                const auto& variableColorType = (*variableColorTypes[variable]);
                if (variableColorType.size() < value) {
                    std::cerr << "Could not find corresponding color id for color index" << value << " in color type " << variableColorType.getName() << std::endl;
                    throw explicit_error(ExplicitErrorType::invalid_trace);
                }
                binding.emplace(variableIt->second, variableColorType[value].getColorName());
            }
            const auto transitionIt = transitionToId.find(traceStep.transition);
            if (transitionIt == transitionToId.end()) {
                std::cerr << "Could not find transition id corresponding to transition with index " << traceStep.transition << std::endl;
                throw explicit_error(ExplicitErrorType::invalid_trace);
            }

            successorGenerator.fire(currentState, traceStep.transition, traceStep.binding);
            std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>> marking;
            for (Place_t place = 0; place < currentState.markings.size(); place++) {
                auto placeIdIt = placeToId.find(place);
                if (placeIdIt == placeToId.end()) {
                    std::cerr << "Could not find corresponding place id for place index" << place << std::endl;
                    throw explicit_error(ExplicitErrorType::invalid_trace);
                }
                std::vector<std::pair<std::vector<std::string>, MarkingCount_t>> tokenCounts;
                for (const auto& [tokenColor, tokenCount] : currentState.markings[place].counts()) {
                    if (tokenCount <= 0) {
                        continue;
                    }
                    std::vector<std::string> colors;
                    const auto underlyingColor = cpnBuilder.getPlaceUnderlyingColorType(place);
                    if (underlyingColor == nullptr) {
                        std::cerr << "Could not find corresponding color type forplace index" << place << std::endl;
                        throw explicit_error(ExplicitErrorType::invalid_trace);
                    }
                    if (const auto productColorType = dynamic_cast<const Colored::ProductType*>(underlyingColor)) {
                        for (size_t colorIndex = 0; colorIndex < net.getPlaces()[place].colorType->colorCodec.getColorCount(); colorIndex++) {
                            colors.push_back((*(*productColorType).getNestedColorType(colorIndex))[colorIndex].getColorName());
                        }
                    } else {
                        colors.push_back((*underlyingColor)[tokenColor].getColorName());
                    }
                    tokenCounts.emplace_back(std::move(colors), tokenCount);
                }
                marking.emplace(placeIdIt->second, std::move(tokenCounts));
            }
            trace.emplace_back(TraceStep {
                transitionIt->second,
                std::move(binding),
                std::move(marking)
            });
        }
        return trace;
    }
}
