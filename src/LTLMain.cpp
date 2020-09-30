#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <utility>
#include <functional>

#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#endif

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "LTL/LTLToBuchi.h"
#include "LTL/LTLValidator.h"
#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

ReturnValue parseModel(AbstractPetriNetBuilder &builder, const std::string &filename) {
    ifstream model_file{filename};
    if (!model_file) {
        fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", filename.c_str());
        fprintf(stdout, "CANNOT_COMPUTE\n");
        return ReturnValue::ErrorCode;
    }

    PNMLParser parser;
    parser.parse(model_file, &builder);
    return ReturnValue::ContinueCode;
}

/**
 * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
 * In the case E f, not f is returned, and in this case the model checking result should be negated
 * (indicated by bool in return value)
 * @param formula - a formula on the form A f, E f or f
 * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
 * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
 */
std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula) {
    LTL::LTLValidator validator;
    bool should_negate = false;
    Condition_ptr converted;
    if (auto _formula = dynamic_cast<ECondition *>(formula.get())) {
        converted = std::make_shared<NotCondition>((*_formula)[0]);
        should_negate = true;
    } else if (auto _formula = dynamic_cast<ACondition *>(formula.get())) {
        converted = (*_formula)[0];
    } else {
        converted = formula;
    }
    converted->visit(validator);
    if (validator.bad()) {
        converted = nullptr;
    }
    return std::make_pair(converted, should_negate);
}

void LTLMain() {
    QueryXMLParser parser;

    std::string qfilename = "/home/waefwerf/dev/P9/INPUTS/AirplaneLD-PT-0200/LTLCardinality.xml";
    std::ifstream queryfile{qfilename};
    assert(queryfile.is_open());
    std::set<size_t> queries{1};
    parser.parse(queryfile, queries);

    PetriNetBuilder builder;
    if (parseModel(builder, "/home/waefwerf/dev/P9/INPUTS/AirplaneLD-PT-0200/model.pnml")) {
        std::unique_ptr<PetriNet> net{builder.makePetriNet()};
        for (const auto &query : parser.queries) {
            if (query.query) {
                auto[negated_formula, negate_answer] = to_ltl(query.query);
                if (!negated_formula) {
                    std::cerr << "Query file " << qfilename << " contained non-LTL formula";
                    exit(1);
                }
                LTL::NestedDepthFirstSearch modelChecker(*net, negated_formula);
                bool satisfied = negate_answer ^ modelChecker.isSatisfied();
                std::cout << "Formula " << (satisfied ? "" : "not ") << "satisfied" << std::endl;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    LTLMain();
/*    QueryXMLParser parser;

    std::ifstream testfile{"test_models/query-test002/query.xml"};
    assert(testfile.is_open());
    std::set<size_t> queries{1};
    parser.parse(testfile, queries);
    //parser.printQueries(2);
    IsCTLVisitor isCtlVisitor;
    parser.queries[1].query->visit(isCtlVisitor);
    cout << "Is CTL query: " << isCtlVisitor.isCTL << endl;
    AsCTL asCtlVisitor;
    parser.queries[1].query->visit(asCtlVisitor);
    cout << "As CTL success." << endl;*/
    return 0;
}

