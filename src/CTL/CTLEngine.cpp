#include "CTL/CTLEngine.h"

#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/CTLResult.h"


#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"


#include "CTL/Stopwatch.h"
#include "PetriEngine/options.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <PetriEngine/PQL/Expressions.h>

using namespace std;
using namespace PetriEngine::PQL;


ReturnValue getAlgorithm(std::shared_ptr<Algorithm::FixedPointAlgorithm>& algorithm,
                         CTL::CTLAlgorithmType algorithmtype, PetriEngine::Reachability::Strategy search)
{
    switch(algorithmtype)
    {
        case CTL::CTLAlgorithmType::Local:
            algorithm = std::make_shared<Algorithm::LocalFPA>(search);
            break;
        case CTL::CTLAlgorithmType::CZero:
            algorithm = std::make_shared<Algorithm::CertainZeroFPA>(search);
            break;
        default:
            cerr << "Error: Unknown or unsupported algorithm" << endl;
            return ErrorCode;
    }
    return ContinueCode;
}

void printResult(const std::string& qname, CTLResult& result, bool statisticslevel, bool mccouput, bool only_stats, size_t index, options_t& options){
    const static string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STATE_COMPRESSION SAT_SMT ";

    if(!only_stats)
    {
        cout << endl;
        cout << "FORMULA "
             << qname
             << " " << (result.result ? "TRUE" : "FALSE") << " "
             << techniques
             << (options.isCPN ? "UNFOLDING_TO_PT " : "")
             << (options.stubbornreduction ? "STUBBORN_SETS " : "")
             << (options.ctlalgorithm == CTL::CZero ? "CTL_CZERO " : "")
             << (options.ctlalgorithm == CTL::Local ? "CTL_LOCAL " : "")
                << endl << endl;
        std::cout << "Query index " << index << " was solved" << std::endl;
        cout << "Query is" << (result.result ? "" : " NOT") << " satisfied." << endl;

        cout << endl;
    }
    if(statisticslevel){
        cout << "STATS:" << endl;
        cout << "	Time (seconds)    : " << setprecision(4) << result.duration / 1000 << endl;
        cout << "	Configurations    : " << result.numberOfConfigurations << endl;
        cout << "	Markings          : " << result.numberOfMarkings << endl;
        cout << "	Edges             : " << result.numberOfEdges << endl;
        cout << "	Processed Edges   : " << result.processedEdges << endl;
        cout << "	Processed N. Edges: " << result.processedNegationEdges << endl;
        cout << "	Explored Configs  : " << result.exploredConfigurations << endl;
        std::cout << endl;
    }
}

bool singleSolve(const Condition_ptr& query, PetriEngine::PetriNet* net,
                 CTL::CTLAlgorithmType algorithmtype,
                 PetriEngine::Reachability::Strategy strategytype, bool partial_order, CTLResult& result)
{
    PetriNets::OnTheFlyDG graph(net, partial_order);
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

bool recursiveSolve(const Condition_ptr& query, PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    PetriEngine::Reachability::Strategy strategytype, bool partial_order, CTLResult& result, options_t& options)
{
    if(auto q = dynamic_cast<NotCondition*>(query.get()))
    {
        return ! recursiveSolve((*q)[0], net, algorithmtype, strategytype, partial_order, result, options);
    }
    else if(auto q = dynamic_cast<AndCondition*>(query.get()))
    {
        return std::all_of(q->begin(), q->end(), [&](auto& q){
            return recursiveSolve(q, net, algorithmtype, strategytype, partial_order, result, options);
        });
    }
    else if(auto q = dynamic_cast<OrCondition*>(query.get()))
    {
        return std::any_of(q->begin(), q->end(), [&](auto& q){
            return recursiveSolve(q, net, algorithmtype, strategytype, partial_order, result, options);
        });
    }
    else if(query->isReachability())
    {
        PetriEngine::Reachability::ReachabilitySearch strategy(nullptr, *net, options.kbound, true);
        std::vector<Condition_ptr> queries{query->prepareForReachability()};
        std::vector<PetriEngine::Reachability::ResultPrinter::Result> res;
        res.emplace_back(PetriEngine::Reachability::ResultPrinter::Unknown);
        auto r = strategy.reachable(queries, res,
                           options.strategy,
                           options.stubbornreduction,
                           false,
                           false,
                           false);
        return  r xor query->isInvariant();
    }
    else
    {
        return singleSolve(query, net, algorithmtype, strategytype, partial_order, result);
    }
}


ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    PetriEngine::Reachability::Strategy strategytype,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput,
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
            PetriNets::OnTheFlyDG graph(net, partial_order);
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
        printResult(querynames[qnum], result, printstatistics, mccoutput, false, qnum, options);
    }
    return SuccessCode;
}

