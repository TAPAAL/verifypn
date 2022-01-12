#include "CTL/CTLEngine.h"

#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/CTLResult.h"

#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"

#include "utils/Stopwatch.h"
#include "PetriEngine/options.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/TAR/TARReachability.h"

#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PrepareForReachability.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

#include <iostream>
#include <iomanip>
#include <vector>

using namespace CTL;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;
using namespace PetriNets;

ReturnValue getAlgorithm(std::shared_ptr<Algorithm::FixedPointAlgorithm>& algorithm,
                         CTLAlgorithmType algorithmtype, Strategy search)
{
    switch(algorithmtype)
    {
        case CTLAlgorithmType::Local:
            algorithm = std::make_shared<Algorithm::LocalFPA>(search);
            break;
        case CTLAlgorithmType::CZero:
            algorithm = std::make_shared<Algorithm::CertainZeroFPA>(search);
            break;
        default:
            cerr << "Error: Unknown or unsupported algorithm" << endl;
            return ErrorCode;
    }
    return ContinueCode;
}

bool singleSolve(Condition* query, PetriNet* net,
                 CTLAlgorithmType algorithmtype,
                 Strategy strategytype, bool partial_order, CTLResult& result);

bool singleSolve(const Condition_ptr& query, PetriNet* net,
                 CTLAlgorithmType algorithmtype,
                 Strategy strategytype, bool partial_order, CTLResult& result)
{
    return singleSolve(query.get(), net, algorithmtype, strategytype, partial_order, result);
}

bool singleSolve(Condition* query, PetriNet* net,
                 CTLAlgorithmType algorithmtype,
                 Strategy strategytype, bool partial_order, CTLResult& result)
{
    OnTheFlyDG graph(net, partial_order);
    graph.setQuery(query);
    std::shared_ptr<Algorithm::FixedPointAlgorithm> alg = nullptr;
    if(getAlgorithm(alg, algorithmtype,  strategytype) == ErrorCode)
    {
        assert(false);
        throw std::exception();
    }

    stopwatch timer;
    timer.start();
    auto res = alg->search(graph);
    timer.stop();

    result.duration += timer.duration();
    result.numberOfConfigurations += graph.configurationCount();
    result.numberOfMarkings += graph.markingCount();
    result.processedEdges += alg->processedEdges();
    result.processedNegationEdges += alg->processedNegationEdges();
    result.exploredConfigurations += alg->exploredConfigurations();
    result.numberOfEdges += alg->numberOfEdges();
    return res;
}

bool recursiveSolve(const Condition_ptr& query, PetriNet* net,
                    CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result, options_t& options);

class ResultHandler : public AbstractHandler {
    private:
        bool _is_conj = false;
        const std::vector<int8_t>& _lstate;
    public:
        ResultHandler(bool is_conj, const std::vector<int8_t>& lstate)
        : _is_conj(is_conj), _lstate(lstate)
        {}

        std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking) override
        {
            if(result == ResultPrinter::Satisfied)
            {
                result = _lstate[index] < 0 ? ResultPrinter::NotSatisfied : ResultPrinter::Satisfied;
            }
            else if(result == ResultPrinter::NotSatisfied)
            {
                result = _lstate[index] < 0 ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
            }
            bool terminate = _is_conj ? (result == ResultPrinter::NotSatisfied) : (result == ResultPrinter::Satisfied);
            return std::make_pair(result, terminate);
        }
};

bool solveLogicalCondition(LogicalCondition* query, bool is_conj, PetriNet* net,
                           CTLAlgorithmType algorithmtype,
                           Strategy strategytype, bool partial_order, CTLResult& result, options_t& options)
{
    std::vector<int8_t> state(query->size(), 0);
    std::vector<int8_t> lstate;
    std::vector<Condition_ptr> queries;
    for(size_t i = 0; i < query->size(); ++i)
    {
        if(PetriEngine::PQL::isReachability((*query)[i]))
        {
            state[i] = dynamic_cast<NotCondition*>((*query)[i].get()) ? -1 : 1;
            queries.emplace_back(prepareForReachability((*query)[i]));
            lstate.emplace_back(state[i]);
        }
    }

    {
        ResultHandler handler(is_conj, lstate);
        std::vector<AbstractHandler::Result> res(queries.size(), AbstractHandler::Unknown);
        if(!options.tar)
        {
            ReachabilitySearch strategy(*net, handler, options.kbound, true);
            strategy.reachable(queries, res,
                                        options.strategy,
                                        options.stubbornreduction,
                                        false,
                                        false,
                                        false,
                                        options.seed());
        }
        else
        {
            TARReachabilitySearch tar(handler, *net, nullptr, options.kbound);
            tar.reachable(queries, res, false, false);
        }
        size_t j = 0;
        for(size_t i = 0; i < query->size(); ++i) {
            if (state[i] != 0)
            {
                if (res[j] == AbstractHandler::Unknown) {
                    ++j;
                    continue;
                }
                auto bres = res[j] == AbstractHandler::Satisfied;

                if(bres xor is_conj) {
                    return !is_conj;
                }
                ++j;
            }
        }
    }

    for(size_t i = 0; i < query->size(); ++i) {
        if (state[i] == 0)
        {
            if(recursiveSolve((*query)[i], net, algorithmtype, strategytype, partial_order, result, options) xor is_conj)
            {
                return !is_conj;
            }
        }
    }
    return is_conj;
}

class SimpleResultHandler : public AbstractHandler
{
public:
    std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking) {
        return std::make_pair(result, false);
    }
};

bool recursiveSolve(Condition* query, PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result, options_t& options);

bool recursiveSolve(const Condition_ptr& query, PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result, options_t& options)
{
    return recursiveSolve(query.get(), net, algorithmtype, strategytype, partial_order, result, options);
}

bool recursiveSolve(Condition* query, PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result, options_t& options)
{
    if(auto q = dynamic_cast<NotCondition*>(query))
    {
        return ! recursiveSolve((*q)[0], net, algorithmtype, strategytype, partial_order, result, options);
    }
    else if(auto q = dynamic_cast<AndCondition*>(query))
    {
        return solveLogicalCondition(q, true, net, algorithmtype, strategytype, partial_order, result, options);
    }
    else if(auto q = dynamic_cast<OrCondition*>(query))
    {
        return solveLogicalCondition(q, false, net, algorithmtype, strategytype, partial_order, result, options);
    }
    else if(PetriEngine::PQL::isReachability(query))
    {
        SimpleResultHandler handler;
        std::vector<Condition_ptr> queries{prepareForReachability(query)};
        std::vector<AbstractHandler::Result> res;
        res.emplace_back(AbstractHandler::Unknown);
        if(options.tar)
        {
            TARReachabilitySearch tar(handler, *net, nullptr, options.kbound);
            tar.reachable(queries, res, false, false);
        }
        else
        {
            ReachabilitySearch strategy(*net, handler, options.kbound, true);
            strategy.reachable(queries, res,
                           options.strategy,
                           options.stubbornreduction,
                           false,
                           false,
                           false,
                           options.seed());
        }
        return (res.back() == AbstractHandler::Satisfied) xor query->isInvariant();
    }
    else
    {
        return singleSolve(query, net, algorithmtype, strategytype, partial_order, result);
    }
}


ReturnValue CTLMain(PetriNet* net,
                    CTLAlgorithmType algorithmtype,
                    Strategy strategytype,
                    bool printstatistics,
                    bool partial_order,
                    const std::vector<std::string>& querynames,
                    const std::vector<std::shared_ptr<Condition>>& queries,
                    const std::vector<size_t>& querynumbers,
                    options_t& options
        )
{
    for(auto qnum : querynumbers){
        CTLResult result(queries[qnum]);
        bool solved = false;

        {
            OnTheFlyDG graph(net, partial_order);
            graph.setQuery(result.query);
            switch (graph.initialEval()) {
                case Condition::Result::RFALSE:
                    result.result = false;
                    solved = true;
                    break;
                case Condition::Result::RTRUE:
                    result.result = true;
                    solved = true;
                    break;
                default:
                    break;
            }
        }
        result.numberOfConfigurations = 0;
        result.numberOfMarkings = 0;
        result.processedEdges = 0;
        result.processedNegationEdges = 0;
        result.exploredConfigurations = 0;
        result.numberOfEdges = 0;
        result.duration = 0;
        if(!solved)
        {
            result.result = recursiveSolve(result.query, net, algorithmtype, strategytype, partial_order, result, options);
        }
        result.print(querynames[qnum], printstatistics, qnum, options, std::cout);
    }
    return SuccessCode;
}

