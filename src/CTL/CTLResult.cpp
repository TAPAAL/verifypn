#include "CTL/CTLResult.h"
#include <iomanip>

void CTLResult::print(const std::string& qname, StatisticsLevel statisticslevel, size_t index, options_t& options, std::ostream& out) const {

    const static std::string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STATE_COMPRESSION SAT_SMT ";

    out << "\n";
    out << "FORMULA "
         << qname
         << " " << (result ? "TRUE" : "FALSE") << " "
         << techniques
         << (options.isCPN ? "UNFOLDING_TO_PT " : "")
         << (options.stubbornreduction ? "STUBBORN_SETS " : "")
         << (options.ctlalgorithm == CTL::CZero ? "CTL_CZERO " : "")
         << (options.ctlalgorithm == CTL::Local ? "CTL_LOCAL " : "")
            << "\n\n";
    out << "Query index " << index << " was solved" << "\n";
    out << "Query is" << (result ? "" : " NOT") << " satisfied." << "\n";

    if(statisticslevel != StatisticsLevel::None){
        out << "\n";
        out << "STATS:" << "\n";
        out << "	Time (seconds)      : " << std::setprecision(4) << duration / 1000 << "\n";
        out << "	Configurations      : " << numberOfConfigurations << "\n";
        out << "	Markings            : " << numberOfMarkings << "\n";
        out << "	Edges               : " << numberOfEdges << "\n";
        out << "	Processed Edges     : " << processedEdges << "\n";
        out << "	Processed N. Edges  : " << processedNegationEdges << "\n";
        out << "	Explored Configs    : " << exploredConfigurations << "\n";
        out << "	Tokens Extrapolated : " << tokensExtrapolated << "\n";
        out << "	max tokens:         : " << maxTokens << "\n"; // kept lower case to be compatible with reachability format
    }
    out << std::endl;
}