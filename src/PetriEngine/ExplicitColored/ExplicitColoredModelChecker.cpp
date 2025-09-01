#include "PetriEngine/ExplicitColored/ExplicitColoredModelChecker.h"
#include <VerifyPN.h>
#include <PetriEngine/Colored/PnmlWriter.h>
#include <PetriEngine/ExplicitColored/ExplicitColoredPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h>
#include <PetriEngine/PQL/Evaluation.h>
#include <utils/NullStream.h>
#include <sstream>
#include <PetriEngine/ExplicitColored/Algorithms/FireabilitySearch.h>

namespace PetriEngine::ExplicitColored {
    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkQuery(
        const Condition_ptr& query,
        options_t& options,
        IColoredResultPrinter* resultPrinter
    ) const {
        std::stringstream pnmlModelStream;
        Result result = Result::UNKNOWN;
        std::ifstream modelFile(options.modelfile);
        pnmlModelStream << modelFile.rdbuf();
        std::string pnmlModel = std::move(pnmlModelStream).str();
        bool isOverApproximationOnly = false;

        if (options.strategy == Strategy::OverApprox) {
            isOverApproximationOnly = true;
            options.strategy = Strategy::RDFS;
        }

        if (options.enablecolreduction) {
            std::stringstream reducedPnml;
            _reduce(pnmlModel, reducedPnml, query, options);
            pnmlModel = std::move(reducedPnml).str();
        }

        if (options.queryReductionTimeout > 0) {
            result = checkColorIgnorantLP(pnmlModel, query, options);
            if (result != Result::UNKNOWN) {
                if (resultPrinter) {
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
        if (isOverApproximationOnly) {
            return Result::UNKNOWN;
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
        auto queryCopy = ConditionCopyVisitor::copyCondition(query);
        ColorIgnorantPetriNetBuilder ignorantBuilder(_stringSet);
        std::stringstream pnmlModelStream {pnmlModel};
        ignorantBuilder.parse_model(pnmlModelStream);

        ignorantBuilder.build();

        auto builder = ignorantBuilder.getUnderlying();
        const auto qnet = std::unique_ptr<PetriNet>(builder.makePetriNet(false));
        const std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
        std::vector queries { std::move(queryCopy) };
        const EvaluationContext context(qm0.get(), qnet.get());

        ContainsFireabilityVisitor hasFireability;
        ContainsDeadlockVisitor hasDeadlock;
        Visitor::visit(hasFireability, queries[0]);
        Visitor::visit(hasDeadlock, queries[0]);
        if (hasDeadlock.getReturnValue()) {
            return Result::UNKNOWN;
        }
        if (hasFireability.getReturnValue()) {
            return checkFireabilityColorIgnorantLP(context, queries, builder, qnet, options);
        }
        return checkCardinalityColorIgnorantLP(context, queries, builder, qnet, qm0, options);
    }

    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkFireabilityColorIgnorantLP(
       const EvaluationContext& context,
       std::vector<std::shared_ptr<Condition>>& queries,
       PetriNetBuilder& builder,
       const std::unique_ptr<PetriNet>& qnet,
       options_t& options
    ) const {
        negstat_t stats;
        queries[0] = pushNegation(queries[0], stats, context, false, false, false);
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
        auto isEf = false;
        EFCondition* q1;
        EFCondition* q2;
        auto queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
        queries.push_back(std::move(queryCopy));
        queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
        queries.push_back(std::move(queryCopy));
        if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(queries[0])) {
            isEf = true;
            q1 = efQuery.get();
            if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(queries[1])) {
                q2 = efQuery.get();
            } else {
                return Result::UNKNOWN;
            }
        }else {
            auto goodQueries = 0;
            isEf = false;
            if (const auto notQuery = std::dynamic_pointer_cast<NotCondition>(queries[0])) {
                if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(notQuery->getCond())) {
                    q1 = efQuery.get();
                    goodQueries++;
                }
            }
            if (auto notQuery = std::dynamic_pointer_cast<NotCondition>(queries[1])) {
                if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(notQuery->getCond())) {
                    q2 = efQuery.get();
                    goodQueries++;
                }
            }
            if (goodQueries < 2) {
                return Result::UNKNOWN;
            }
        }
        //Copying transforms compare conjunction to fireable, so we need to analyse for safety
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
        //If the first query is satisfied then the original query is satisfied (reverse if it is an AG query)
        queries[0] = pushNegation(std::make_shared<AGCondition>(q1->getCond()), stats, context, false, false, false);
        //If the second query isn't satisfied then the original query is not satisfied (reverse if it is an AG query)
        //Equal to satisfying AG not e
        queries[1] = pushNegation(std::make_shared<EFCondition>(*q2), stats, context, false, false, false);
        //Just for input
        std::vector<std::string> names;
        ResultPrinter printer(&builder, &options, names);
        //Unknown result means not computed, Ignore means it cannot be computed
        std::vector results(queries.size(), ResultPrinter::Result::Unknown);
        FireabilitySearch strategy(*qnet, printer, options.queryReductionTimeout);
        strategy.reachable(queries, results,
                                Strategy::RDFS,
                                false,
                                false,
                                StatisticsLevel::None,
                                options.trace != TraceLevel::None,
                                options.seed()
                                );
        if (results[0] == ResultPrinter::Satisfied) {
            return isEf ? Result::SATISFIED : Result::UNSATISFIED;
        }
        if (results[1] == ResultPrinter::NotSatisfied) {
            return isEf ? Result::UNSATISFIED : Result::SATISFIED;
        }
        return Result::UNKNOWN;
    }

    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkCardinalityColorIgnorantLP(
        const EvaluationContext& context,
        std::vector<std::shared_ptr<Condition>>& queries,
        PetriNetBuilder& builder,
        const std::unique_ptr<PetriNet>& qnet,
        const std::unique_ptr<MarkVal[]>& qm0,
        options_t& options
    ) const {
        negstat_t stats;
        queries[0] = pushNegation(queries[0], stats, context, false, false, false);
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
        auto isEf = false;
        auto queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
        queries.push_back(std::move(queryCopy));
        if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(queries[1])) {
            isEf = true;
            auto innerQuery = efQuery->getCond();
            queries[1] = pushNegation(std::make_shared<AGCondition>(innerQuery), stats, context, false, false, false);
        }else {
            if (const auto notQuery = std::dynamic_pointer_cast<NotCondition>(queries[1])) {
                if (auto efQuery = std::dynamic_pointer_cast<EFCondition>(notQuery->getCond())) {
                    auto innerQuery = efQuery->getCond();
                    queries[1] = pushNegation(std::make_shared<AGCondition>(innerQuery), stats, context, false, false, false);
                }
            }
        }
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
        const auto r = evaluate(queries[0].get(), context);
        const auto r2 = evaluate(queries[1].get(), context);

        if(r == Condition::RFALSE) {
            queries[0] = BooleanCondition::FALSE_CONSTANT;
        }
        else if(r == Condition::RTRUE) {
            queries[0] = BooleanCondition::TRUE_CONSTANT;
        }
        if(r2 == Condition::RFALSE) {
            queries[1] = BooleanCondition::FALSE_CONSTANT;
        }
        else if(r2 == Condition::RTRUE) {
            queries[1] = BooleanCondition::TRUE_CONSTANT;
        }
        NullStream nullStream;
        simplify_queries(qm0.get(), qnet.get(), queries, options, nullStream);
        if (queries[0] == BooleanCondition::TRUE_CONSTANT && !isEf) {
            return Result::SATISFIED;
        }
        if (queries[0] == BooleanCondition::FALSE_CONSTANT && isEf) {
            return Result::UNSATISFIED;
        }
        if (queries[1] == BooleanCondition::TRUE_CONSTANT && !isEf) {
            return Result::UNSATISFIED;
        }
        if (queries[1] == BooleanCondition::TRUE_CONSTANT && isEf) {
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
        ExplicitColoredPetriNetBuilder cpnBuilder;
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
            _fullStatisticOut << "Could not do colored reductions" << std::endl;
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
        const ExplicitColoredPetriNetBuilder& cpnBuilder,
        const ColoredPetriNet& net
    ) const {
        const auto& variableColorTypes = cpnBuilder.getUnderlyingVariableColorTypes();

        ColoredSuccessorGenerator successorGenerator(net);
        auto currentState = net.initial();
        std::vector<TraceStep> trace;

        trace.emplace_back(_translateMarking(currentState, cpnBuilder, net));

        for (auto& traceStep : internalTrace) {
            Binding traceBinding;

            successorGenerator.findNextValidBinding(
                currentState,
                traceStep.transition,
                traceStep.binding,
                net.getTotalBindings(traceStep.transition),
                traceBinding,
                0
            );

            successorGenerator.shrinkState(0);

            std::unordered_map<std::string, std::string> binding;
            for (auto [variable, value] : traceBinding.getValues()) {
                if (variable >= cpnBuilder.getVariableCount()) {
                    std::cerr << "Could not find variable id corresponding to variable index " << variable << std::endl;
                    throw explicit_error(ExplicitErrorType::INVALID_TRACE);
                }
                const auto variableName = cpnBuilder.getVariableName(variable);

                if (variable >= variableColorTypes.size()) {
                    std::cerr << "Could not find color type for variable index " << variable << "(" << variableName << ")" << std::endl;
                    throw explicit_error(ExplicitErrorType::INVALID_TRACE);
                }
                const auto& variableColorType = (*variableColorTypes[variable]);

                if (variableColorType.size() < value) {
                    std::cerr << "Could not find corresponding color id for color index" << value << " in color type " << variableColorType.getName() << std::endl;
                    throw explicit_error(ExplicitErrorType::INVALID_TRACE);
                }
                binding.emplace(variableName, variableColorType[value].getColorName());
            }

            if (traceStep.transition >= cpnBuilder.getTransitionCount()) {
                std::cerr << "Could not find transition id corresponding to transition with index " << traceStep.transition << std::endl;
                throw explicit_error(ExplicitErrorType::INVALID_TRACE);
            }
            auto transitionName = cpnBuilder.getTransitionName(traceStep.transition);

            successorGenerator.fire(currentState, traceStep.transition, traceBinding);
            currentState.shrink();
            auto translatedMarking = _translateMarking(currentState, cpnBuilder, net);
            trace.emplace_back(transitionName, std::move(binding), std::move(translatedMarking));
        }
        return trace;
    }

    std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>>
        ExplicitColoredModelChecker::_translateMarking(
            const ColoredPetriNetMarking& marking,
            const ExplicitColoredPetriNetBuilder& cpnBuilder,
            const ColoredPetriNet& net
        ) const {
        std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>> translatedMarking;
        for (Place_t place = 0; place < marking.markings.size(); place++) {
            if (place >= cpnBuilder.getPlaceCount()) {
                std::cerr << "Could not find corresponding place id for place index" << place << std::endl;
                throw explicit_error(ExplicitErrorType::INVALID_TRACE);
            }
            const auto& placeName = cpnBuilder.getPlaceName(place);

            std::vector<std::pair<std::vector<std::string>, MarkingCount_t>> tokenCounts;
            for (const auto& [tokenColor, tokenCount] : marking.markings[place].counts()) {
                if (tokenCount <= 0) {
                    continue;
                }
                std::vector<std::string> colors;
                const auto underlyingColor = cpnBuilder.getPlaceUnderlyingColorType(place);
                if (underlyingColor == nullptr) {
                    std::cerr << "Could not find corresponding color type for place index" << place << std::endl;
                    throw explicit_error(ExplicitErrorType::INVALID_TRACE);
                }
                if (const auto productColorType = dynamic_cast<const Colored::ProductType*>(underlyingColor)) {
                    for (size_t colorIndex = 0; colorIndex < net.getPlaces()[place].colorType->colorCodec.getColorCount(); colorIndex++) {
                        auto color = net.getPlaces()[place].colorType->colorCodec.decode(tokenColor, colorIndex);
                        colors.push_back((*(*productColorType).getNestedColorType(colorIndex))[color].getColorName());
                    }
                } else {
                    colors.push_back((*underlyingColor)[tokenColor].getColorName());
                }
                tokenCounts.emplace_back(std::move(colors), tokenCount);
            }
            translatedMarking.emplace(placeName, std::move(tokenCounts));
        }
        return translatedMarking;
    }
}
