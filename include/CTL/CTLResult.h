#ifndef CTLRESULT_H
#define CTLRESULT_H

#include "../PetriEngine/errorcodes.h"
#include "../PetriEngine/PQL/PQL.h"

#include <string>

struct CTLResult {
    CTLResult(const PetriEngine::PQL::Condition_ptr& qry){
        query = qry;
    }

    PetriEngine::PQL::Condition_ptr query;
    bool result;

    double duration = 0;
    size_t numberOfMarkings = 0;
    size_t numberOfConfigurations = 0;
    size_t processedEdges = 0;
    size_t processedNegationEdges = 0;
    size_t exploredConfigurations = 0;
    size_t numberOfEdges = 0;
#ifdef VERIFYPNDIST
    size_t numberOfRoundsComputingDistance = 0;
    size_t numberOfTokensReceived = 0;
    size_t numberOfRequestsReceived = 0;
    size_t numberOfAnswersReceived = 0;
    size_t numberOfMessagesSend = 0;
#endif
};

#endif // CTLRESULT_H
