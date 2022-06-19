/* TAPAAL untimed verification engine verifypn
 * Copyright (C) 2011-2021  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>,
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * CTL Extension
 *                          Peter Fogh <peter.f1992@gmail.com>
 *                          Isabella Kaufmann <bellakaufmann93@gmail.com>
 *                          Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>
 *                          Lasse Steen Jensen <lassjen88@gmail.com>
 *                          Søren Moss Nielsen <soren_moss@mac.com>
 *                          Samuel Pastva <daemontus@gmail.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * Stubborn sets, query simplification, siphon-trap property
 *                          Frederik Meyer Boenneland <sadpantz@gmail.com>
 *                          Jakob Dyhr <jakobdyhr@gmail.com>
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *                          Mads Johannsen <mads_johannsen@yahoo.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * LTL Extension
 *                          Nikolaj Jensen Ulrik <nikolaj@njulrik.dk>
 *                          Simon Mejlby Virenfeldt <simon@simwir.dk>
 *
 * Color Extension
 *                          Alexander Bilgram <alexander@bilgram.dk>
 *                          Peter Haar Taankvist <ptaankvist@gmail.com>
 *                          Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                          Andreas H. Klostergaard
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
// #include <filesystem>
// #include <bits/stdc++.h>
// #include <iostream>
// #include <sys/stat.h>
// #include <sys/types.h>
#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#include <mutex>
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
#include "PetriEngine/PQL/CTLVisitor.h"
#include "PetriEngine/PQL/XMLPrinter.h"
#include "PetriEngine/PQL/FormulaSize.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/BinaryPrinter.h"
#include "PetriEngine/PQL/Simplifier.h"
#include "PetriEngine/PQL/PushNegation.h"
#include "PetriEngine/PQL/PrepareForReachability.h"

#include "PetriEngine/options.h"
#include "utils/errors.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "PetriEngine/Colored/Unfolder.h"

#include "PetriEngine/TraceReplay.h"


#include <atomic>


using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;
#ifndef VERIFYPN_H
#define VERIFYPN_H


bool reduceColored(ColoredPetriNetBuilder &cpnBuilder, std::vector<std::shared_ptr<PQL::Condition> > &queries,
                   TemporalLogic logic, uint32_t timeout, std::ostream &out, int reductiontype,std::vector<uint32_t>& reductions);

std::tuple<PetriNetBuilder, shared_name_name_map, shared_place_color_map>
unfold(ColoredPetriNetBuilder& cpnBuilder, bool compute_partiton, bool compute_symmetry, bool computed_fixed_point,
    std::ostream& out = std::cout, int32_t partitionTimeout = 0, int32_t max_intervals = 0, int32_t intervals_reduced = 0, int32_t interval_timeout = 0, bool over_approx = false);

ReturnValue contextAnalysis(bool colored, const shared_name_name_map& transition_names, const shared_place_color_map& place_names, PetriNetBuilder& builder, const PetriNet* net, std::vector<std::shared_ptr<Condition> >& queries);
std::vector<Condition_ptr > readQueries(shared_string_set& string_set, options_t& options, std::vector<std::string>& qstrings);
void printStats(PetriNetBuilder& builder, options_t& options);
void writeQueries(const std::vector<std::shared_ptr<Condition>>& queries, std::vector<std::string>& querynames, std::vector<uint32_t>& order,
    std::string& filename, bool binary, const shared_name_index_map& place_names, bool keep_solved, bool compact = false);
std::vector<Condition_ptr> getCTLQueries(const std::vector<Condition_ptr>& ctlStarQueries);
std::vector<Condition_ptr> getLTLQueries(const std::vector<Condition_ptr>& ctlStarQueries);
Condition_ptr simplify_ltl_query(Condition_ptr query,
                                 options_t options,
                                 const EvaluationContext &evalContext,
                                 SimplificationContext &simplificationContext,
                                 std::ostream &out = std::cout);

void outputNet(const PetriNetBuilder &builder, std::string out_file);
void outputQueries(const PetriNetBuilder &builder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
        std::vector<std::string> &querynames, std::string filename, uint32_t binary_query_io, bool keep_solved);

void outputCompactQueries(const PetriNetBuilder &builder, const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
    std::vector<std::string> &querynames, std::string filenamem, bool keep_solved);

void simplify_queries(const MarkVal* marking,
                      const PetriNet* net,
                        std::vector<PetriEngine::PQL::Condition_ptr>& queries,
                        options_t& options, std::ostream& outstream = std::cout);


std::vector<Condition_ptr>
parseXMLQueries(shared_string_set& string_set, std::vector<std::string>& qstrings, std::istream& qfile, const std::set<size_t>& qnums, bool binary = false);

#endif /* VERIFYPN_H */

