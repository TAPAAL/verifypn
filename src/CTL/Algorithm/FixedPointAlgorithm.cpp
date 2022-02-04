#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/SearchStrategy/BFSSearch.h"
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/SearchStrategy/HeuristicSearch.h"

namespace Algorithm {
    FixedPointAlgorithm::FixedPointAlgorithm(Strategy type) {
        using namespace PetriEngine::Reachability;
        using namespace SearchStrategy;
        switch(type)
        {
            case Strategy::DFS:
                strategy = std::make_shared<DFSSearch>();
                break;
            case Strategy::RDFS:
                strategy = std::make_shared<RDFSSearch>();
                break;
            case Strategy::BFS:
                strategy = std::make_shared<BFSSearch>();
                break;
            case Strategy::HEUR:
                strategy = std::make_shared<HeuristicSearch>();
                break;
            default:
                throw base_error("Search strategy is unsupported by the CTL-Engine");
        }
    }


}
