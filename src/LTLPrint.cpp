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

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/QueryBinaryParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"
#include "PetriEngine/PQL/CTLVisitor.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "LTL/LTLToBuchi.h"

#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>

using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;


int main(int argc, char* argv[]) {
    QueryXMLParser parser;

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
    cout << "As CTL success." << endl;
    return 0;
}

