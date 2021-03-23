
#ifndef QUERYPARSER_H
#define QUERYPARSER_H


#include "PetriEngine/PQL/PQL.h"

struct QueryItem {
    std::string id; // query name
    PetriEngine::PQL::Condition_ptr query;

    enum {
        PARSING_OK,
        UNSUPPORTED_QUERY,
    } parsingResult;
};


#endif /* QUERYPARSER_H */

