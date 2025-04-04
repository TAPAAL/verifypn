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
        ContainsFireabilityVisitor hasFireability;
        Visitor::visit(hasFireability, queries[0]);
        Condition::Result r;
        bool isEf = false;

        //Fireability check
        if (hasFireability.getReturnValue()) {
            negstat_t stats;
            queries[0] = pushNegation(queries[0], stats, context, false, false, false);
            contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
            try {
                EFCondition* q1;
                EFCondition* q2;
                queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
                queries.push_back(std::move(queryCopy));
                queryCopy = ConditionCopyVisitor::copyCondition(queries[0]);
                queries.push_back(std::move(queryCopy));
                if (dynamic_cast<EFCondition*>(queries[0].get())) {
                    isEf = true;
                    q1 = dynamic_cast<EFCondition*>(queries[0].get());
                    q2 = dynamic_cast<EFCondition*>(queries[1].get());
                }else {
                    isEf = false;
                    q1 = dynamic_cast<EFCondition*>(dynamic_cast<NotCondition*>(queries[0].get())->getCond().get());
                    q2 = dynamic_cast<EFCondition*>(dynamic_cast<NotCondition*>(queries[1].get())->getCond().get());
                }
                //Copying transforms compare conjunction to fireable, so we need to analyse for safety
                contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
                //If the first query is satisfied then the original query is satisfied (reverse if it is an AG query)
                queries[0] = std::make_shared<AGCondition>(q1->getCond());
                //If the second query isn't satisfied then the original query is not satisfied (reverse if it is an AG query)
                //Equal to satisfying AG not e
                queries[1] = std::make_shared<EFCondition>(*q2);
                queries[0] = pushNegation(queries[0], stats, context, false, false, false);
                queries[1] = pushNegation(queries[2], stats, context, false, false, false);
            } catch (std::exception&)  {
                return Result::UNKNOWN;
            }
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

        //Cardinality check
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);
        if (dynamic_cast<EFCondition*>(queries[0].get())) {
            isEf = true;
        }

        r = evaluate(queries[0].get(), context);

        if(r == Condition::RFALSE) {
            queries[0] = BooleanCondition::FALSE_CONSTANT;
        }
        else if(r == Condition::RTRUE) {
            queries[0] = BooleanCondition::TRUE_CONSTANT;
        }
        simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);
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
