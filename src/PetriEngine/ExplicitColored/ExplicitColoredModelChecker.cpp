#include "PetriEngine/ExplicitColored/ExplicitColoredModelChecker.h"
#include <VerifyPN.h>
#include <PetriEngine/Colored/PnmlWriter.h>
#include <PetriEngine/ExplicitColored/ColoredPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h>
#include <PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h>
#include <PetriEngine/PQL/Evaluation.h>
#include <utils/NullStream.h>
#include <sstream>
#include <PetriEngine/ExplicitColored/Algorithms/FireabilitySearch.h>

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
        throw explicit_error(ExplicitErrorType::unsupported_query);
        SearchStatistics searchStatistics;
        result = explicitColorCheck(pnmlModel, query, options, &searchStatistics);
        if (result != Result::UNKNOWN) {
            if (resultPrinter) {
                resultPrinter->printResult(searchStatistics, result == Result::SATISFIED
                    ? AbstractHandler::Result::Satisfied
                    : AbstractHandler::Result::NotSatisfied
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
        const auto status = ignorantBuilder.build();
        if (status == ColoredIgnorantPetriNetBuilderStatus::CONTAINS_NEGATIVE) {
            return Result::UNKNOWN;
        }

        auto builder = ignorantBuilder.getUnderlying();
        auto qnet = std::unique_ptr<PetriNet>(builder.makePetriNet(false));
        const std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());

        std::vector queries { std::move(queryCopy) };

        const EvaluationContext context(qm0.get(), qnet.get());
        negstat_t stats;
        ContainsFireabilityVisitor hasFireability;
        std::cout << "Query before reduction: " << std::endl;
        bool isEf = true;
        // if (dynamic_cast<EFCondition*>(queries[0].get())) {
        //     isEf = true;
        // }
        queries[0]->toString(std::cout);
        queries[0] = pushNegation(queries[0], stats, context, false, false, false);
        Visitor::visit(hasFireability, queries[0]);
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);

        std::cout << std::endl << "Query after reduction: " << std::endl;
        queries[0]->toString(std::cout);
        Condition::Result r;

        if (hasFireability.getReturnValue()) {
            try {
                queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
                queries.push_back(std::move(queryCopy));
                EFCondition* q1;// = dynamic_cast<EFCondition*>(queries[0].get());
                EFCondition* q2;// = dynamic_cast<EFCondition*>(queries[1].get());
                if (dynamic_cast<EFCondition*>(queries[0].get())) {
                    q1 = dynamic_cast<EFCondition*>(queries[0].get());
                    q2 = dynamic_cast<EFCondition*>(queries[1].get());
                }else {
                    if (isEf) {
                        isEf = false;
                    }
                    q1 = dynamic_cast<EFCondition*>(dynamic_cast<NotCondition*>(queries[0].get())->getCond().get());
                    q2 = dynamic_cast<EFCondition*>(dynamic_cast<NotCondition*>(queries[1].get())->getCond().get());
                }
                contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
                queries[0] = std::make_shared<AGCondition>(q1->getCond());
                queries[1] = std::make_shared<EFCondition>(*q2);
                queries[0] = pushNegation(queries[0], stats, context, false, false, false);
                queries[1] = pushNegation(queries[1], stats, context, false, false, false);
                // std::cout << std::endl << "Query 'check if true': \n";
                // queries[0]->toString(std::cout);
                // std::cout << std::endl << "Query 'check if false': \n";
                // queries[1]->toString(std::cout);
                // std::cout << std::endl;
            } catch (std::exception&)  {
                return Result::UNKNOWN;
            }

            //Just for input
            std::vector<std::string> names = {"Check if true", "Check if false"};
            ResultPrinter printer(&builder, &options, names);

            std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
            FireabilitySearch strategy(*qnet, printer, options.kbound);
            strategy.reachable(queries, results,
                                    Strategy::RDFS,
                                    false,
                                    false,
                                    StatisticsLevel::None,
                                    options.trace != TraceLevel::None,
                                    options.seed(),
                                    options.depthRandomWalk,
                                    options.incRandomWalk);
            if (results[0] == ResultPrinter::Satisfied) {
                return isEf ? Result::SATISFIED : Result::UNSATISFIED;
            }
            if (results[1] == ResultPrinter::NotSatisfied) {
                return isEf ? Result::UNSATISFIED : Result::SATISFIED;
            }
            return Result::UNKNOWN;
        }else {
            r = evaluate(queries[0].get(), context);
        }

        if(r == Condition::RFALSE) {
            queries[0] = BooleanCondition::FALSE_CONSTANT;
        }
        else if(r == Condition::RTRUE) {
            queries[0] = BooleanCondition::TRUE_CONSTANT;
        }
        simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);
        std::cout << std::endl << "Query is " << (isEf ? "EF " : "AG ") << "and " << r << std::endl;
        if (queries[0] == BooleanCondition::FALSE_CONSTANT && isEf) {
            return Result::UNSATISFIED;
        }
        if (queries[0] == BooleanCondition::TRUE_CONSTANT && !isEf) {
            return Result::SATISFIED;
        }
        return Result::UNKNOWN;
    }

    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::explicitColorCheck(
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
                return Result::UNKNOWN;
            default:
                throw base_error("Unknown builder error ", static_cast<uint32_t>(buildStatus));
        }

        auto net = cpnBuilder.takeNet();
        auto placeIndices = cpnBuilder.takePlaceIndices();
        auto transitionIndices = cpnBuilder.takeTransitionIndices();

        ExplicitWorklist worklist(net, query, placeIndices, transitionIndices, options.seed());
        bool result = worklist.check(options.strategy, options.colored_sucessor_generator);

        if (searchStatistics) {
            *searchStatistics = worklist.GetSearchStatistics();
        }
        return result ? Result::SATISFIED : Result::UNSATISFIED;
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

}
