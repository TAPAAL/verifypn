#ifndef CTLRESULT_H
#define CTLRESULT_H

#include "CTLParser/CTLQuery.h"
#include "../PetriEngine/errorcodes.h"

#include <string>

struct CTLResult {
    CTLResult(CTLQuery* qry,
              const std::string& model,
              size_t qnbr,
              bool printstat,
              bool printmcc){
        query = qry;
        modelName = model;
        queryNumber = qnbr;
        printStatistics = printstat;
        printMccOutput = printmcc;
    }

    std::string modelName;
    size_t queryNumber;
    CTLQuery *query;
    bool result;

    bool printStatistics;
    bool printMccOutput;

    double duration;
    size_t numberOfMarkings;
    size_t numberOfConfigurations;
};

#endif // CTLRESULT_H
