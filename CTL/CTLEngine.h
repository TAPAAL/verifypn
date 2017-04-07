#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "../PetriEngine/errorcodes.h"
#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/Reachability/ReachabilitySearch.h"

#include "Algorithm/AlgorithmTypes.h"

#include <set>

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    char* queryfile,
                    CTL::CTLAlgorithmType algorithmtype,
                    PetriEngine::Reachability::Strategy strategytype,
                    std::set<size_t> querynumbers,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput);

#endif // CTLENGINE_H
