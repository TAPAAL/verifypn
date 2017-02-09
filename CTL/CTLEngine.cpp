#include "CTLEngine.h"

#include "CTLParser/CTLQuery.h"
#include "CTLParser/CTLParser.h"
#include "PetriNets/OnTheFlyDG.h"
#include "CTLResult.h"
#include "DependencyGraph/Edge.h"

#include "SearchStrategy/WaitingList.h"
#include "SearchStrategy/DFSSearch.h"
#include "SearchStrategy/UniversalSearchStrategy.h"

#include "Algorithm/CertainZeroFPA.h"
#include "Algorithm/LocalFPA.h"

#include "Stopwatch.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

ReturnValue parseQueries(const char *filename,
                         PetriEngine::PetriNet* net,
                         vector<CTLQuery*> &queries,
                         QueryMeta &meta){
    ifstream xmlfile(filename);

    if(!xmlfile.is_open()){
        cerr << "Error: Query file \"" << filename << "\"" << "could not be opened" << endl;
        return ErrorCode;
    }

    vector<char> buffer((istreambuf_iterator<char>(xmlfile)), istreambuf_iterator<char>());
    buffer.push_back('\0');

    CTLParser qparser = CTLParser();
    meta = *qparser.GetQueryMetaData(buffer);

    for(int i = 0; i < meta.numberof_queries; ++i){
        auto q = qparser.ParseXMLQuery(buffer, i + 1);
        q = qparser.FormatQuery(q, net);
        queries.push_back(q);
    }

    return ContinueCode;
}

ReturnValue makeCTLResults(vector<CTLResult>& results,
                           const vector<CTLQuery*>& queries,
                           const QueryMeta& meta,
                           std::set<size_t>& querynumbers,
                           bool printstatistics,
                           bool mccoutput)
{
    if(querynumbers.empty()){
        for(size_t i = 0; i < queries.size(); ++i)
            querynumbers.insert(i);
    }

    for(auto qnbr: querynumbers){
        if(qnbr > queries.size() || qnbr < 0){
            cerr << "Error: Invalid query number. Requested " << qnbr << " out of " << queries.size() << " queries" << endl;
            return ErrorCode;
        }

        CTLQuery* q = queries[qnbr];
        CTLResult r(q,
                     meta.model_name,
                     qnbr,
                     printstatistics,
                     mccoutput);
        results.push_back(r);
    }

    return ContinueCode;
}

ReturnValue getStrategy(SearchStrategy::iSequantialSearchStrategy*& strategy,
                        PetriEngine::Reachability::Strategy strategytype)
{
    if(strategytype == PetriEngine::Reachability::DFS){
        strategy = new SearchStrategy::DFSSearch();
    }
    else if(strategytype == PetriEngine::Reachability::BFS){
        strategy = new SearchStrategy::UniversalSearchStrategy<SearchStrategy::WaitingList<DependencyGraph::Edge*, std::queue<DependencyGraph::Edge*>>>();
    }
    else {
        cerr << "Error: Unknown strategy" << endl;
        return ErrorCode;
    }
    return ContinueCode;
}

void printResult(CTLResult& result, bool statisticslevel, bool mccouput){
    const static string techniques = "TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT";
    const string resultstring = result.result ? "TRUE" : "FALSE";

    if(statisticslevel){
        cout << result.modelName << "-" << result.queryNumber << endl;
        cout << "   [Total Eval. Time]   " << result.duration << " ms" << endl;
        cout << "   [No. Configurations] " << result.numberOfConfigurations << endl;
        cout << "   [No. Markings]       " << result.numberOfMarkings << endl;
        cout << "   [Query Number]       " << result.queryNumber + 1 << endl;
        cout << "   [Query Result]       " << resultstring << endl;
    }
    else if(!statisticslevel && !mccouput){
        cout << result.modelName << "-" << result.queryNumber << " " << resultstring << endl;
    }
    if(mccouput){
        cout << "FORMULA "
             << result.modelName
             << "-" << result.queryNumber - 1
             << resultstring << " "
             << techniques
             << endl << endl;
    }
}

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    char* queryfile,
                    CTL::CTLAlgorithmType algorithmtype,
                    PetriEngine::Reachability::Strategy strategytype,
                    std::set<size_t> querynumbers,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput)
{
    vector<CTLQuery*> queries;
    vector<CTLResult> results;
    QueryMeta meta;
    PetriNets::OnTheFlyDG& graph = *new PetriNets::OnTheFlyDG(net);

    if(parseQueries(queryfile, net, queries, meta) == ErrorCode) return ErrorCode;
    if(makeCTLResults(results, queries, meta, querynumbers, printstatistics, mccoutput) == ErrorCode) return ErrorCode;

    stopwatch totaltimer;
    double totaltime = 0;
    totaltimer.start();
    for(CTLResult& result : results){

        SearchStrategy::iSequantialSearchStrategy* strategy = nullptr;
        if(getStrategy(strategy, strategytype) == ErrorCode) return ErrorCode;

        graph.setQuery(result.query);

        stopwatch timer;
        timer.start();
        if(algorithmtype == CTL::CTLAlgorithmType::Local){
            result.result = Algorithm::LocalFPA().search(graph, *strategy);
        }
        else if (algorithmtype == CTL::CTLAlgorithmType::CZero){
            result.result = Algorithm::CertainZeroFPA().search(graph, *strategy);
        }
        timer.stop();

        result.numberOfConfigurations = graph.configurationCount();
        result.numberOfMarkings = graph.markingCount();
        totaltime += result.duration = timer.duration();

        printResult(result, printstatistics, mccoutput);
    }
    totaltimer.stop();

    if(printstatistics){
        cout << "[Total Eval. Time]              " << totaltimer.duration() << " ms" << endl
             << "[Total Eval. Time w/o clean-up] " << totaltimer.duration() << " ms" << endl;
    }

    return SuccessCode;
}
