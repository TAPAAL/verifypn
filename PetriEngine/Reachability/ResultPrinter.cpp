
#include "ReachabilityResult.h"
#include "../PetriNetBuilder.h"
#include "../options.h"

namespace PetriEngine {
    namespace Reachability {
        ResultPrinter::Result ResultPrinter::printResult(
                size_t index,
                PQL::Condition* query, 
                ResultPrinter::Result result,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                const std::vector<size_t> enabledTransitionsCount,
                int maxTokens,
                const std::vector<uint32_t> maxPlaceBound, Structures::StateSetInterface* stateset,
                size_t lastmarking)
        {
            if(result == Unknown) return Unknown;
            Result retval = result;
            std::cout << std::endl;    
            
            bool showTrace = (result == Satisfied);
            
            if(options->mccoutput)
            {
                if(!options->statespaceexploration && retval != Unknown)
                {
                    std::cout << "FORMULA " << querynames[index] << " ";
                }
                else {
                    retval = Satisfied;
                    uint32_t placeBound = 0;
                    for (size_t p = 0; p < maxPlaceBound.size(); p++) {
                        placeBound = std::max<uint32_t>(placeBound, maxPlaceBound[p]);
                    }
                    // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
                    std::cout   << "STATE_SPACE STATES "<< exploredStates           << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
    //                            << "STATE_SPACE TRANSITIONS "<< discoveredStates <<" TECHNIQUES EXPLICIT\n" 
                                << "STATE_SPACE TRANSITIONS "<< -1                  << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
                                << "STATE_SPACE MAX_TOKEN_PER_MARKING "<< maxTokens << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
                                << "STATE_SPACE MAX_TOKEN_IN_PLACE "<< placeBound   << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n"
                                << std::endl;
                    return retval;
                }

                if (result == Satisfied)
                    retval = query->isInvariant() ? NotSatisfied : Satisfied;
                else if (result == NotSatisfied)
                    retval = query->isInvariant() ? Satisfied : NotSatisfied;

                //Print result
                if (retval == Unknown)
                {
                    std::cout << "\nUnable to decide if " << querynames[index] << " is satisfied.";
                }
                else if (retval == Satisfied) {
                    if(!options->statespaceexploration)
                    {
                        std::cout << "TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";
    /*                    if(options->enablereduction > 0)
                        {*/
                            std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                    }
                        std::cout << std::endl;
                    }
                } else if (retval == NotSatisfied) {
                    if (!query->placeNameForBound().empty()) {
                        // find index of the place for reporting place bound

                        std::cout << query->getBound() <<  " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";

    //                    if(options->enablereduction > 0)
    //                    {
                            std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                    }
                    } else {
                        if(!options->statespaceexploration)
                        {
                            std::cout << "FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";
    //                        if(options->enablereduction > 0)
    //                        {
                                std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                        }
                        }
                    }
                }
            }
            else
            {
                std::cout << "Query is ";
                if(options->statespaceexploration)
                {
                    retval = Satisfied;
                }

                if (result == Satisfied)
                    retval = query->isInvariant() ? NotSatisfied : Satisfied;
                else if (result == NotSatisfied)
                    retval = query->isInvariant() ? Satisfied : NotSatisfied;

                //Print result
                if (retval == Unknown)
                {
                    std::cout << "MAYBE ";
                }
                else if (retval == NotSatisfied) {
                    std::cout << "NOT ";
                }
                std::cout << "satisfied." << std::endl;
                
            }
            
            if(showTrace && options->trace)
            {
                if(stateset == NULL)
                {
                    std::cout << "No trace could be generated, try disabeling the overapproximaton" << std::endl;
                }
                else
                {
                    printTrace(stateset, lastmarking);                        
                }
            }
            
            std::cout << std::endl;
            return retval;
        }
        
        void ResultPrinter::printTrace(Structures::StateSetInterface* ss, size_t lastmarking)
        {
            std::cerr << "Trace:\n<trace>\n";
            std::stack<size_t> transitions;
            size_t next = lastmarking;
            while(next != 0) // assume 0 is the index of the first marking.
            {
                // (parent, transition)
                std::pair<size_t, size_t> p = ss->getHistory(next);
                next = p.first;
                transitions.push(p.second);
            }
            
            if(reducer != NULL)
                reducer->initFire(std::cerr);
            
            while(transitions.size() > 0)
            {
                size_t trans = transitions.top();
                transitions.pop();
                std::string tname = ss->net().transitionNames()[trans];
                std::cerr << "\t<transition id=\"" << tname << "\">\n";
                
                // well, yeah, we are not really efficient in constructing the trace.
                // feel free to improve
                for(size_t p = 0; p < ss->net().numberOfPlaces(); ++p)
                {
                    size_t cnt = ss->net().inArc(p, trans);
                    for(size_t token = 0; token < cnt; ++token )
                    {
                        std::cerr << "\t\t<token place=\"" << ss->net().placeNames()[p] << "\" age=\"0\"/>\n";
                    }
                }
                
                if(reducer != NULL)
                    reducer->extraConsume(std::cerr, tname);
                
                std::cerr << "\t</transition>\n";
                
                if(reducer != NULL)
                    reducer->postFire(std::cerr, tname);
                
            }
            
            std::cerr << "</trace>\n" << std::endl;
        }

    }
}