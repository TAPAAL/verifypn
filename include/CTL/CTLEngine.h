#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "utils/errors.h"
#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/options.h"

#include "Algorithm/AlgorithmTypes.h"
#include "../PetriEngine/PQL/PQL.h"

#include "CTLResult.h"

#include <set>

bool CTLSingleSolve(PetriEngine::PQL::Condition* query, PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    Strategy strategytype, bool partial_order, CTLResult& result);

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    CTL::CTLAlgorithmType algorithmtype,
                    Strategy strategytype,
                    StatisticsLevel printstatistics,
                    bool partial_order,
                    TokenEliminationMethod token_elim_method,
                    const std::vector<std::string>& querynames,
                    const std::vector<std::shared_ptr<PetriEngine::PQL::Condition>>& reducedQueries,
                    const std::vector<size_t>& ids,
                    options_t& options);

#endif // CTLENGINE_H
