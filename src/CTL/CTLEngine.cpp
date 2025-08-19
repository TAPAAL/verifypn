#include "CTL/CTLEngine.h"

#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/CTLResult.h"

#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"

#include "utils/stopwatch.h"
#include "PetriEngine/options.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/TAR/TARReachability.h"

#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PrepareForReachability.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "LTL/LTLSearch.h"

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
            throw base_error("Unknown or unsupported algorithm");
    }
    return ReturnValue::ContinueCode;
}

bool CTLSingleSolve(const Condition_ptr& query, PetriNet* net,
                 CTLAlgorithmType algorithmtype,
                 Strategy strategytype, bool partial_order, CTLResult& result)
{
    return CTLSingleSolve(query.get(), net, algorithmtype, strategytype, partial_order, result);
}

bool CTLSingleSolve(Condition* query, PetriNet* net,
                 CTLAlgorithmType algorithmtype,
                 Strategy strategytype, bool partial_order, CTLResult& result)
{
    TokenEliminator token_elim;
    OnTheFlyDG graph(net, partial_order, *token_elim.setEnabled(false));
    graph.setQuery(query);
    std::shared_ptr<Algorithm::FixedPointAlgorithm> alg = nullptr;
    getAlgorithm(alg, algorithmtype,  strategytype);

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
    result.tokensEliminated = graph.tokensEliminated();
    result.maxTokens = std::max(graph.maxTokens(), result.maxTokens);
    return res;
}

bool recursiveSolve(const Condition_ptr& query, PetriNet* net,
                    CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result, options_t& options);

class SimpleResultHandler : public AbstractHandler
{
public:
    size_t _expanded = 0;
    size_t _explored = 0;
    size_t _discovered = 0;
    size_t _max_tokens = 0;
    size_t _stored = 0;

    std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking, bool) {
        _expanded = std::max(_expanded, expandedStates);
        _explored = std::max(_explored, exploredStates);
        _discovered = std::max(_discovered, discoveredStates);
        _max_tokens = std::max<size_t>(_max_tokens, maxTokens);
        _stored = std::max(_stored, stateset->size());
        return std::make_pair(result, false);
    }
};

class ResultHandler : public SimpleResultHandler {
    private:
        bool _is_conj = false;
        const std::vector<int8_t>& _lstate;
    public:
        ResultHandler(bool is_conj, const std::vector<int8_t>& lstate)
        : SimpleResultHandler(), _is_conj(is_conj), _lstate(lstate)
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
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking, bool) override
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
            SimpleResultHandler::handle(index, query, result, maxPlaceBound, expandedStates, exploredStates, discoveredStates, maxTokens, stateset, lastmarking, initialMarking, false);
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
                               StatisticsLevel::None,
                               false,
                               options.seed());
            result.maxTokens = std::max(handler._max_tokens, result.maxTokens);
            result.exploredConfigurations += handler._explored;
            result.numberOfConfigurations += handler._stored;
            result.numberOfMarkings += handler._stored;
        }
        else
        {
            TARReachabilitySearch tar(handler, *net, nullptr, options.kbound);
            tar.reachable(queries, res, StatisticsLevel::None, false);
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
            tar.reachable(queries, res, StatisticsLevel::None, false);
        }
        else
        {
            ReachabilitySearch strategy(*net, handler, options.kbound, true);
            strategy.reachable(queries, res,
                               options.strategy,
                               options.stubbornreduction,
                               false,
                               StatisticsLevel::None,
                               false,
                               options.seed());
            result.maxTokens = std::max(handler._max_tokens, result.maxTokens);
            result.exploredConfigurations += handler._explored;
            result.numberOfConfigurations += handler._stored;
            result.numberOfMarkings += handler._stored;
        }
        return (res.back() == AbstractHandler::Satisfied) xor query->isInvariant();
    }
    else if(!containsNext(query)) {
        // there are probably many more cases w. nested quantifiers we can do
        // one instance is E[ non_temp U [E non_temp U ...]] in a chain
        // also, this should go into some *neat* visitor to do the check.
        auto q = query->shared_from_this();
        bool ok = false;
        if(auto* af = dynamic_cast<AFCondition*>(query))
        {
            if(!isTemporal((*af)[0]))
                ok = true;
        }
        else if(auto* eg = dynamic_cast<EGCondition*>(query))
        {
            if(!isTemporal((*eg)[0]))
                ok = true;
        }
        else if(auto* au = dynamic_cast<AUCondition*>(query)) {
            if(!isTemporal((*au)[0]) && !isTemporal((*au)[1]))
                ok = true;
        }
        else if(auto* eu = dynamic_cast<EUCondition*>(query)) {
            if(!isTemporal((*eu)[0]) && !isTemporal((*eu)[1]))
                ok = true;
        }
        if(ok)
        {
            LTL::LTLSearch search(*net, q, options.buchiOptimization, options.ltl_compress_aps);
            auto r = search.solve(false, options.kbound, options.ltlalgorithm, options.ltl_por,
                            options.strategy, options.ltlHeuristic, options.ltluseweak, options.seed_offset);
            result.numberOfMarkings += search.markings();
            result.numberOfConfigurations += search.configurations();
            result.exploredConfigurations += search.explored();
            result.maxTokens = std::max(search.max_tokens(), result.maxTokens);
            return r;
        }
    }
    //else
    {
        return CTLSingleSolve(query, net, algorithmtype, strategytype, partial_order, result);
    }
}


ReturnValue CTLMain(PetriNet* net,
                    CTLAlgorithmType algorithmtype,
                    Strategy strategytype,
                    StatisticsLevel printstatistics,
                    bool partial_order,
                    const TokenEliminationMethod token_elim_method,
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
            TokenEliminator token_elim;
            token_elim.setDynamic(token_elim_method == TokenEliminationMethod::Dynamic);
            token_elim.setEnabled(token_elim_method == TokenEliminationMethod::Disabled);

            OnTheFlyDG graph(net, partial_order, token_elim);
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
        result.maxTokens = 0;
        if(!solved)
        {
            if(options.strategy == Strategy::BFS || options.strategy == Strategy::RDFS)
                result.result = CTLSingleSolve(result.query, net, algorithmtype, options.strategy, options.stubbornreduction, result);
            else
                result.result = recursiveSolve(result.query, net, algorithmtype, strategytype, partial_order, result, options);
        }
        result.print(querynames[qnum], printstatistics, qnum, options, std::cout);
    }
    return ReturnValue::SuccessCode;
}

