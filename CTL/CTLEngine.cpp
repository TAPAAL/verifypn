#include "CTLEngine.h"

#include "PetriNets/OnTheFlyDG.h"
#include "CTLResult.h"
#include "DependencyGraph/Edge.h"

#include "SearchStrategy/DFSSearch.h"

#include "Algorithm/CertainZeroFPA.h"
#include "Algorithm/LocalFPA.h"

#include "PetriEngine/PQL/PQL.h"

#include "Stopwatch.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

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

void printResult(const std::string& qname, CTLResult& result, bool statisticslevel, bool mccouput, bool only_stats){
    const static string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION STUBBORN_SETS";

    if(!only_stats)
    {
        cout << endl;
        cout << "FORMULA "
             << qname
             << " " << (result.result ? "TRUE" : "FALSE") << " "
             << techniques
             << endl << endl;

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
        if(!only_stats) result.query->toString(cout);
        std::cout << endl;
    }
}

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    PetriEngine::Reachability::Strategy strategytype,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput,
                    const std::vector<std::string>& querynames,
                    const std::vector<std::shared_ptr<Condition>>& queries,
                    const std::vector<size_t>& querynumbers
        )
{

    if(strategytype != PetriEngine::Reachability::DFS){
        std::cerr << "Error: Invalid CTL search strategy. Only DFS is supported by CTL engine." << std::endl;
        return ErrorCode;
    }

    size_t isfirst = true;
    
    for(auto qnum : querynumbers){
        CTLResult result(queries[qnum]);
        PetriNets::OnTheFlyDG graph(net);        
        auto q = result.query->pushNegation(); // stupid way to make a copy
        graph.setQuery(q);

        std::shared_ptr<Algorithm::FixedPointAlgorithm> alg = nullptr;

        if(getAlgorithm(alg, algorithmtype,  strategytype) == ErrorCode)
        {
            return ErrorCode;
        }

        stopwatch timer;
        timer.start();
        result.result = alg->search(graph);
        timer.stop();

        result.duration = timer.duration();
        result.numberOfConfigurations = graph.configurationCount();
        result.numberOfMarkings = graph.markingCount();
        result.processedEdges = alg->processedEdges();
        result.processedNegationEdges = alg->processedNegationEdges();
        result.exploredConfigurations = alg->exploredConfigurations();
        result.numberOfEdges = alg->numberOfEdges();
        printResult(querynames[qnum], result, printstatistics, mccoutput, false);
    }
    return SuccessCode;
}
