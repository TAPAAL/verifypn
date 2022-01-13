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
#include "utils/stopwatch.h"
#include "CTL/CTLResult.h"
#include "GameSuccessorGenerator.h"

#include <vector>
#include <memory>
#include <inttypes.h>


#ifndef REACHABILITYSYNTHESIS_H
#define REACHABILITYSYNTHESIS_H

namespace PetriEngine {
    namespace Synthesis {

        std::pair<bool, PQL::Condition*> get_predicate(PQL::Condition* condition);

        class SimpleSynthesis {
        public:

            SimpleSynthesis(PetriNet& net, PQL::Condition& query, size_t kbound = 0);

            ~SimpleSynthesis();

            Reachability::ResultPrinter::Result synthesize(
                    Strategy strategy,
                    bool use_stubborn,
                    bool permissive);

            void print_strategy(std::ostream& strategy_out);
            const CTLResult& result() { return _result; }

        private:
            using successors_t = std::vector<std::pair<size_t, SynthConfig*>>;

            bool eval(PetriEngine::PQL::Condition* cond, const PetriEngine::MarkVal* marking);

            bool check_bound(const PetriEngine::MarkVal* marking);

            void dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back);

#ifndef NDEBUG
            void print_id(size_t);
            void validate(PetriEngine::PQL::Condition*, PetriEngine::Structures::AnnotatedStateSet<SynthConfig>&, bool is_safety);
#endif

            void run(bool use_stubborn, Strategy strategy, bool permissive);

            SynthConfig& get_config(Structures::State& marking, PQL::Condition* prop, size_t& cid);
            std::pair<bool, successors_t> get_env_successors(GameSuccessorGenerator& generator, SynthConfig& cconf);
            std::tuple<bool, bool, successors_t> get_ctrl_successors(GameSuccessorGenerator& generator, SynthConfig& cconf, const bool permissive, const bool env_empty);
            void fix_assignment(SynthConfig& cconf, const bool some_ctrl, const bool some_winning, const bool ctrl_empty,
            const bool some_env, const bool env_empty);
            void to_queue(Structures::Queue& q, successors_t& successors, bool is_ctrl, SynthConfig& cconf);

            size_t _kbound;
            PetriNet& _net;
            Structures::State _working;
            Structures::State _parent;
            Structures::AnnotatedStateSet<SynthConfig> _stateset;
            bool _is_safety = false;
            PQL::Condition& _query;
            PQL::Condition* _predicate = nullptr;
            CTLResult _result;

        };
    }
}

#endif /* REACHABILITYSYNTHESIS_H */

