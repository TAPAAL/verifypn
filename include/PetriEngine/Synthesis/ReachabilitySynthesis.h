/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (C) 2019 Peter G. Jensen <root@petergjoel.dk>
 */

/*
 * File:   ReachabilitySynthesis.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 8, 2019, 2:19 PM
 */
#include "SynthConfig.h"
#include "PetriEngine/options.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"
#include "utils/Stopwatch.h"
#include "CTL/CTLResult.h"


#include <vector>
#include <memory>
#include <inttypes.h>


#ifndef REACHABILITYSYNTHESIS_H
#define REACHABILITYSYNTHESIS_H

namespace PetriEngine {
    namespace Synthesis {

        class ReachabilitySynthesis {
        private:
            Reachability::ResultPrinter& printer;

        public:

            ReachabilitySynthesis(Reachability::ResultPrinter& printer, PetriNet& net, size_t kbound = 0);

            ~ReachabilitySynthesis();

            Reachability::ResultPrinter::Result synthesize(
                    PQL::Condition& query,
                    Strategy strategy,
                    bool use_stubborn = false,
                    bool keep_strategies = false,
                    bool permissive = false,
                    std::ostream* strategy_out = nullptr);
        private:


            bool eval(PetriEngine::PQL::Condition* cond, const PetriEngine::MarkVal* marking);

            bool check_bound(const PetriEngine::MarkVal* marking);

            size_t dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety);

            void print_strategy(std::ostream& strategy_out,
                    PetriEngine::Structures::AnnotatedStateSet<SynthConfig>& stateset, SynthConfig& meta, bool is_safety);

#ifndef NDEBUG
            void print_id(size_t);
            void validate(PetriEngine::PQL::Condition*, PetriEngine::Structures::AnnotatedStateSet<SynthConfig>&, bool is_safety);
#endif

            void run(PQL::Condition* query, bool is_safety, CTLResult& result, bool permissive, std::ostream* strategy_out);

            SynthConfig& get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, Structures::State& marking, PQL::Condition* prop, bool is_safety, size_t& cid);

            size_t _kbound;
            PetriNet& _net;
        };
    }
}

#endif /* REACHABILITYSYNTHESIS_H */

