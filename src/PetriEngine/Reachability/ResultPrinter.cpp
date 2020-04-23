
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/options.h"
#include "PetriEngine/PQL/Expressions.h"

namespace PetriEngine {
    namespace Reachability {
        std::pair<AbstractHandler::Result, bool> ResultPrinter::handle(
                size_t index,
                PQL::Condition* query, 
                Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,                
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking)
        {
            if(result == Unknown) return std::make_pair(Unknown,false);

            Result retval = result;
            
            if(options->cpnOverApprox)
            {
                if(query->getQuantifier() == PQL::Quantifier::UPPERBOUNDS)
                {
                    auto upq = ((PQL::UnfoldedUpperBoundsCondition*)query);
                    auto bnd = upq->bounds();
                    if(initialMarking == nullptr || bnd > upq->value(initialMarking))
                        retval = Unknown;
                }
                else if (result == Satisfied)
                    retval = query->isInvariant() ? Unknown : Unknown;
                else if (result == NotSatisfied)
                    retval = query->isInvariant() ? Satisfied : NotSatisfied;                
                if(retval == Unknown)
                {
                    std::cout << "\nUnable to decide if " << querynames[index] << " is satisfied.\n\n";
                    std::cout << "Query is MAYBE satisfied.\n" << std::endl;
                    return std::make_pair(Ignore,false);
                }
            }
            std::cout << std::endl;    
            
            bool showTrace = (result == Satisfied);
            
            if(!options->statespaceexploration && retval != Unknown)
            {
                std::cout << "FORMULA " << querynames[index] << " ";
            }
            else {
                retval = Satisfied;
                uint32_t placeBound = 0;
                if(maxPlaceBound != nullptr)
                {
                    for (size_t p = 0; p < maxPlaceBound->size(); p++) {
                        placeBound = std::max<uint32_t>(placeBound, (*maxPlaceBound)[p]);
                    }
                }
                // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
                std::cout   << "STATE_SPACE STATES "<< exploredStates           << " " << techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE TRANSITIONS "<< -1                  << " " << techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE MAX_TOKEN_PER_MARKING "<< maxTokens << " " << techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE MAX_TOKEN_IN_PLACE "<< placeBound   << " " << techniquesStateSpace 
                            << std::endl;
                return std::make_pair(retval,false);
            }

            if (result == Satisfied)
                retval = query->isInvariant() ? NotSatisfied : Satisfied;
            else if (result == NotSatisfied)
                retval = query->isInvariant() ? Satisfied : NotSatisfied;           

            //Print result
            auto bound = query;
            if(auto ef = dynamic_cast<PQL::EFCondition*>(query))
            {
                bound = (*ef)[0].get();
            }
            bound = dynamic_cast<PQL::UnfoldedUpperBoundsCondition*>(bound);
            
            if (retval == Unknown)
            {
                std::cout << "\nUnable to decide if " << querynames[index] << " is satisfied.";
            }
            else if(bound)
            {
                std::cout << ((PQL::UnfoldedUpperBoundsCondition*)bound)->bounds() << " " << techniques << printTechniques() << std::endl;
                std::cout << "Query index " << index << " was solved" << std::endl;
            }
            else if (retval == Satisfied) {
                if(!options->statespaceexploration)
                {
                    std::cout << "TRUE " << techniques << printTechniques() << std::endl;
                    std::cout << "Query index " << index << " was solved" << std::endl;
                }
            } else if (retval == NotSatisfied) {
                if(!options->statespaceexploration)
                {
                    std::cout << "FALSE " << techniques << printTechniques() << std::endl;
                    std::cout << "Query index " << index << " was solved" << std::endl;
                }
            }
            
            std::cout << std::endl;

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
            
            if(options->cpnOverApprox)
                std::cout << "\nSolved using CPN Approximation\n" << std::endl;
            
            if(showTrace && options->trace)
            {
                if(stateset == nullptr)
                {
                    std::cout << "No trace could be generated" << std::endl;
                }
                else
                {
                    printTrace(stateset, lastmarking);                        
                }
            }
            
            std::cout << std::endl;
            return std::make_pair(retval, false);
        }
        
        std::string ResultPrinter::printTechniques() {
            std::string out;
                        
            if(options->queryReductionTimeout > 0)
            {
                out += "LP_APPROX ";
            }

            if(options->cpnOverApprox)
            {
                out += "CPN_APPROX ";
            }
            
            if(options->isCPN && !options->cpnOverApprox)
            {
                out += "UNFOLDING_TO_PT ";
            }
            
            if(options->queryReductionTimeout == 0 
			    && !options->tar 
			    && options->siphontrapTimeout == 0)
            {
                out += "EXPLICIT STATE_COMPRESSION ";
                if(options->stubbornreduction)
                {
                    out += "STUBBORN_SETS ";
                }
            }
            if(options->tar)
            {
                out += "TRACE_ABSTRACTION_REFINEMENT ";
            }
            if(options->siphontrapTimeout > 0)
            {
                out += "TOPOLOGICAL SIPHON_TRAP ";
            }
            
            return out;
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
                std::cerr << "\t<transition id=\"" << tname << "\" index=\"" << trans << "\">\n";
                
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
