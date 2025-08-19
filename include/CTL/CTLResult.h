#ifndef CTLRESULT_H
#define CTLRESULT_H

#include "utils/errors.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"


#include <ostream>
#include <string>

struct CTLResult {
    CTLResult(PetriEngine::PQL::Condition* qry)
    : query(qry) {};

    CTLResult(const PetriEngine::PQL::Condition_ptr& qry)
    : query(qry.get()) {}

    PetriEngine::PQL::Condition* query;
    bool result;

    double duration = 0;
    size_t numberOfMarkings = 0;
    size_t numberOfConfigurations = 0;
    size_t processedEdges = 0;
    size_t processedNegationEdges = 0;
    size_t exploredConfigurations = 0;
    size_t numberOfEdges = 0;
    size_t tokensEliminated = 0;
    size_t maxTokens = 0;
#ifdef VERIFYPNDIST
    size_t numberOfRoundsComputingDistance = 0;
    size_t numberOfTokensReceived = 0;
    size_t numberOfRequestsReceived = 0;
    size_t numberOfAnswersReceived = 0;
    size_t numberOfMessagesSend = 0;
#endif
    void print(const std::string& qname, StatisticsLevel statisticslevel, size_t index, options_t& options, std::ostream& out) const;
};

#endif // CTLRESULT_H
