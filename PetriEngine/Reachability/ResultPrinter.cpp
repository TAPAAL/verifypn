
#include "ReachabilityResult.h"
#include "../PetriNetBuilder.h"
#include "../options.h"

namespace PetriEngine {
    namespace Reachability {
        ResultPrinter::Result ResultPrinter::printResult(
                size_t index,
                PQL::Condition* query, 
                ResultPrinter::Result result,
                const std::string& explanation,
                BigInt expandedStates,
                BigInt exploredStates,
                BigInt discoveredStates,
                const std::vector<BigInt> enabledTransitionsCount,
                int maxTokens,
                const std::vector<unsigned int> maxPlaceBound )
        {
            if(result == Unknown) return Unknown;
            Result retval = result;
            std::cout << std::endl;            
            if(!options->statespaceexploration)
            {
                std::cout << "FORMULA " << querynames[index] << " ";
            }
            else {
                retval = Satisfied;
                unsigned int placeBound = 0;
                for (size_t p = 0; p < maxPlaceBound.size(); p++) {
                    placeBound = std::max<unsigned int>(placeBound, maxPlaceBound[p]);
                }
                // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
                std::cout   << "STATE_SPACE STATES "<< exploredStates <<" TECHNIQUES EXPLICIT\n" 
//                            << "STATE_SPACE TRANSITIONS "<< discoveredStates <<" TECHNIQUES EXPLICIT\n" 
                            << "STATE_SPACE TRANSITIONS "<< -1 <<" TECHNIQUES EXPLICIT\n" 
                            << "STATE_SPACE MAX_TOKEN_PER_MARKING "<< maxTokens << " TECHNIQUES EXPLICIT\n" 
                            << "STATE_SPACE MAX_TOKEN_IN_PLACE "<< placeBound <<" TECHNIQUES EXPLICIT\n"
                            << std::endl;
                return retval;
            }
            
            if (result == Satisfied)
                retval = query->isInvariant() ? NotSatisfied : Satisfied;
            else if (result == NotSatisfied)
                retval = query->isInvariant() ? Satisfied : NotSatisfied;

            //Print result
            if (retval == Unknown)
                std::cout << "\nUnable to decide if query is satisfied.\n\n";
            else if (retval == Satisfied) {
                if(!options->statespaceexploration)
                {
                    std::cout << "TRUE TECHNIQUES EXPLICIT ";
                    if(options->enablereduction > 0)
                    {
                        std::cout << "STRUCTURAL_REDUCTION";
                    }
                    std::cout << std::endl;
                }
                std::cout << "\nQuery is satisfied.\n\n";
            } else if (retval == NotSatisfied) {
                if (!query->placeNameForBound().empty()) {
                    // find index of the place for reporting place bound
                    for(auto& p : query->placeNameForBound())
                    {
                        uint32_t pi = builder->getPlaceNames().at(p);
                        std::cout << maxPlaceBound[pi] << " ";
                    }
                    std::cout <<  "TECHNIQUES EXPLICIT ";
                    
                    if(options->enablereduction > 0)
                    {
                        std::cout << "STRUCTURAL_REDUCTION";
                    }
/*                    std::cout << "\n\nMaximum number of tokens in place "<<
                            query->placeNameForBound() << ": " <<
                            maxPlaceBound[p] << "\n\n"; */
                } else {
                    if(!options->statespaceexploration)
                    {
                        std::cout << "FALSE TECHNIQUES EXPLICIT ";
                        if(options->enablereduction > 0)
                        {
                            std::cout << "STRUCTURAL_REDUCTION";
                        }
                        std::cout << "\n";
                    }
                    std::cout << "\nQuery is NOT satisfied.\n\n";
                }
            }
            std::cout << std::endl;
            return retval;
        }            

    }
}