#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/SearchStrategy/BFSSearch.h"
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/SearchStrategy/HeuristicSearch.h"

namespace Algorithm {
    FixedPointAlgorithm::FixedPointAlgorithm(PetriEngine::Reachability::Strategy type) {
        using namespace PetriEngine::Reachability;
        using namespace SearchStrategy;
        switch(type)
        {
            case DFS:
                strategy = std::make_shared<DFSSearch>();
                break;
            case RDFS:
                strategy = std::make_shared<RDFSSearch>();
                break;
            case BFS:
                strategy = std::make_shared<BFSSearch>();
                break;
            case HEUR:
                strategy = std::make_shared<HeuristicSearch>();
                break;
            default:
                std::cerr << "Search strategy is unsupported by the CTL-Engine"   <<  std::endl;
                assert(false);
                exit(ErrorCode);                
        }
    }


}
