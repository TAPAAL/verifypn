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
 * File:   ReachabilitySynthesis.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 8, 2019, 2:19 PM
 */

#include "PetriEngine/Synthesis/SimpleSynthesis.h"
#include "PetriEngine/Synthesis/SynthConfig.h"
#include "PetriEngine/options.h"
#include "utils/Stopwatch.h"
#include "CTL/CTLResult.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

#include <vector>


namespace PetriEngine {
    using namespace Reachability;

    namespace Synthesis {

        SimpleSynthesis::SimpleSynthesis(PetriNet& net, PQL::Condition& query, size_t kbound)
        : _kbound(kbound), _net(net), _working(net.makeInitialMarking()), _parent(_net.makeInitialMarking()),
            _stateset(_net, 0), _query(query), _result(&query) {

        }

        SimpleSynthesis::~SimpleSynthesis() {
        }

        std::pair<bool, PQL::Condition*> get_predicate(PQL::Condition* condition)
        {

            if(auto a = dynamic_cast<PQL::ACondition*>(condition))
            {
                if(auto p = dynamic_cast<PQL::FCondition*>((*a)[0].get()))
                {
                    return {false, (*p)[0].get()};
                }
                else if (auto p = dynamic_cast<PQL::GCondition*>((*a)[0].get()))
                {
                    return {true, (*p)[0].get()};
                }
            }
            else if(auto a = dynamic_cast<PQL::AGCondition*>(condition))
            {
                return {true, (*a)[0].get()};
            }
            else if(auto a = dynamic_cast<PQL::AFCondition*>(condition))
            {
                return {false, (*a)[0].get()};
            }
            throw base_error("ERROR: Only AF and AG propositions supported for synthesis");

        }

        ResultPrinter::Result SimpleSynthesis::synthesize(
            Strategy strategy,
            bool use_stubborn,
            bool permissive) {
            using namespace Structures;


            auto ctrl = dynamic_cast<PQL::ControlCondition*> (&_query);
            if(ctrl == nullptr)
                throw base_error("ERROR: Missing topmost control-condition for synthesis");
            auto quant = dynamic_cast<PQL::SimpleQuantifierCondition*>((*ctrl)[0].get());
            auto [_is_safety, predicate] = get_predicate(quant);
            if (PQL::isTemporal(predicate))
                throw base_error("ERROR: Only simple synthesis propositions supported (i.e. one top-most AF or AG and no other nested quantifiers)");

            run(predicate, strategy, permissive);

            //printer.printResult(result);
            return _result.result ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
        }

        void SimpleSynthesis::dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back) {
            size_t processed = 0;
            //std::cerr << "BACK[" << next->_marking << "]" << std::endl;
            //std::cerr << "Win ? " << SynthConfig::state_to_str(next->_state) << std::endl;
            for (auto& dep : next->_dependers) {
                ++processed;

                SynthConfig* ancestor = dep.second;
                //std::cerr << "\tBK[" << ancestor->_marking << "] : " << (int) ancestor->_state << " (" << ancestor->_ctrl_children << "/" << ancestor->_env_children << ")" << std::endl;
                if (ancestor->determined())
                    continue;
                bool ctrl_child = dep.first;
                if (ctrl_child) {
                    //std::cerr << "\tCB[" << ancestor->_marking << "]" << std::endl;
                    ancestor->_ctrl_children -= 1;
                    if (next->_state == SynthConfig::WINNING) {
                        if (ancestor->_env_children == 0) {
                            //std::cerr << "WIN [" << ancestor->_marking << "]" << std::endl;
                            assert(ancestor->_state != SynthConfig::LOSING);
                            ancestor->_state = SynthConfig::WINNING;
                        } else {
                            ancestor->_state = SynthConfig::MAYBE;
                        }
                    }


                    if (ancestor->_ctrl_children == 0 && // no more tries, and no potential or certainty
                        (ancestor->_state & (SynthConfig::MAYBE | SynthConfig::WINNING)) == 0) {
                        assert(ancestor->_state != SynthConfig::WINNING);
                        ancestor->_state = SynthConfig::LOSING;
                    }
                } else {
                    //std::cerr << "\tEB[" << ancestor->_marking << "]" << std::endl;
                    ancestor->_env_children -= 1;
                    if (next->_state == SynthConfig::LOSING) {
                        assert(ancestor->_state != SynthConfig::WINNING);
                        ancestor->_state = SynthConfig::LOSING;
                    } else if (next->_state == SynthConfig::WINNING) {
                        if (ancestor->_env_children == 0 && ancestor->_state == SynthConfig::MAYBE) {
                            assert(ancestor->_state != SynthConfig::LOSING);
                            ancestor->_state = SynthConfig::WINNING;
                        }
                    }
                }

                if (ancestor->_env_children == 0 && ancestor->_ctrl_children == 0 && ancestor->_state == SynthConfig::MAYBE) {
                    assert(ancestor->_state != SynthConfig::LOSING);
                    ancestor->_state = SynthConfig::WINNING;
                }

                if (ancestor->determined()) {
                    //std::cerr << "BET [" << ancestor->_marking << "] : " << (int)ancestor->_state << std::endl;
                    if (ancestor->_waiting < 2) {
                        back.push(ancestor);
                    }
                    ancestor->_waiting = 2;

                }
            }
            next->_dependers.clear();
            _result.processedEdges += processed;
        }

        bool SimpleSynthesis::check_bound(const MarkVal* marking) {
            if (_kbound > 0) {
                size_t sum = 0;
                for (size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    sum += marking[p];
                if (_kbound < sum)
                    return false;
            }
            return true;
        }

        bool SimpleSynthesis::eval(PQL::Condition* cond, const MarkVal* marking) {
            PQL::EvaluationContext ctx(marking, &_net);
            return cond->evaluate(ctx) == PQL::Condition::RTRUE;
            // TODO, we can use the stability in the fixpoint computation to prun the Dep-graph
        }

#ifndef NDEBUG
        std::vector<MarkVal*> markings;
#endif

        SynthConfig& SimpleSynthesis::get_config(Structures::State& state, PQL::Condition* prop, size_t& cid) {
            // TODO, we don't actually have to store winning markings here (what is fastest, checking query or looking up marking?/memory)!
            auto res = _stateset.add(state);
            cid = res.second;
            SynthConfig& meta = _stateset.get_data(res.second);
            {
#ifndef NDEBUG
                Structures::State tmp(new MarkVal[_net.numberOfPlaces()]);
                _stateset.decode(tmp, res.second);
                std::memcmp(tmp.marking(), state.marking(), sizeof (MarkVal) * _net.numberOfPlaces());
#endif
            }

            if (res.first) {
                ++_result.numberOfConfigurations;
                ++_result.numberOfMarkings;
#ifndef NDEBUG
                markings.push_back(new MarkVal[_net.numberOfPlaces()]);
                memcpy(markings.back(), state.marking(), sizeof (MarkVal) * _net.numberOfPlaces());
#endif
                meta = {SynthConfig::UNKNOWN, false, 0, 0, SynthConfig::depends_t(), res.second};
                if (!check_bound(state.marking())) {
                    meta._state = SynthConfig::LOSING;
                } else {
                    auto res = eval(prop, state.marking());
                    if (_is_safety) {
                        if (res == true) // flipped by reductions, so negate result here!
                            meta._state = SynthConfig::LOSING;
                    } else {
                        if (res != false)
                            meta._state = SynthConfig::WINNING;
                    }
                }
            }


            return meta;
        }

#ifndef NDEBUG

        void SimpleSynthesis::print_id(size_t id) {
            std::cerr << "[" << id << "] : ";
            Structures::State s(markings[id]);
            s.print(_net, std::cerr);
            s.release();
        }


        // validating the solution of the DEP graph (reachability-query is assumed)

        void SimpleSynthesis::validate(PQL::Condition* query, Structures::AnnotatedStateSet<SynthConfig>& stateset, bool is_safety) {
            Structures::State working(new MarkVal[_net.numberOfPlaces()]);
            size_t old = markings.size();
            for (size_t id = 0; id < old; ++id) {
                std::cerr << "VALIDATION " << id << std::endl;
                auto& conf = stateset.get_data(id);
                if (conf._state != SynthConfig::WINNING &&
                    conf._state != SynthConfig::LOSING &&
                    conf._state != SynthConfig::MAYBE &&
                    conf._state != SynthConfig::PROCESSED)
                    continue;
                PQL::EvaluationContext ctx(markings[id], &_net);
                auto res = query->evaluate(ctx);
                if (conf._state != SynthConfig::WINNING)
                    assert((res != PQL::Condition::RTRUE) == is_safety);
                else if (res != is_safety) {
                    assert(conf._state == SynthConfig::WINNING);
                    continue;
                }
                SuccessorGenerator generator(_net);
                Structures::State s(markings[id]);
                generator.prepare(&s);
                bool ok = false;
                std::vector<size_t> env_maybe;
                std::vector<size_t> env_win;
                while (generator.next_env(working)) {
                    auto res = stateset.add(working);
                    if (!res.first) {
                        auto& c = stateset.get_data(res.second);
                        if (c._state == SynthConfig::LOSING) {
                            assert(conf._state == SynthConfig::LOSING);
                            std::cerr << "[" << id << "] -E-> [" << res.second << "]\n";
                            ok = true;
                            break;
                        } else if (c._state != SynthConfig::WINNING) {
                            env_maybe.push_back(res.second);
                        } else {
                            std::cerr << "[" << id << "] -E_W-> [" << res.second << "]\n";
                            env_win.push_back(res.second);
                        }
                    }
                }
                s.release();
                if (!ok) {
                    if (env_maybe.size() > 0) {
                        assert(conf._state != SynthConfig::WINNING);
                        for (auto i : env_maybe) {
                            std::cerr << "[" << id << "] -E-> [" << i << "]\n";
                        }
                        continue;
                    }
                    std::vector<size_t> not_win;
                    bool some = false;
                    while (generator.next_ctrl(working)) {
                        some = true;
                        auto res = stateset.add(working);
                        if (!res.first) {
                            auto& c = stateset.get_data(res.second);
                            if (c._state == SynthConfig::WINNING) {
                                std::cerr << "[" << id << "] -C-> [" << res.second << "]\n";
                                std::cerr << (int) c._state << " :: " << c._marking << std::endl;
                                std::cerr << "ANCESTOR " << (int) conf._state << " :: " << conf._marking << std::endl;

                                assert(conf._state == SynthConfig::WINNING);
                                ok = true;
                                break;
                            } else {
                                not_win.push_back(res.second);
                            }
                        }
                    }
                    if (!some) {
                        assert((conf._state == SynthConfig::WINNING) != env_win.empty() || query->isInvariant());
                    } else {
                        assert(ok || conf._state != SynthConfig::WINNING);
                        if (!ok) {
                            for (auto i : not_win) {
                                std::cerr << "[" << id << "] -C-> [" << i << "]\n";
                            }
                        }
                    }
                }
            }
        }
#endif

        void SimpleSynthesis::print_strategy(std::ostream& out) {
            std::stack<size_t> missing;

            Structures::State parent(_net.makeInitialMarking());
            Structures::State working(_net.makeInitialMarking());
            {
                auto res = _stateset.add(working);
                missing.emplace(res.second);
            }
            if (&out == &std::cout) out << "\n##BEGIN STRATEGY##\n";
            out << "{\n";
            SuccessorGenerator generator(_net);
            bool first_marking = true;
            while (!missing.empty()) {
                auto nxt = missing.top();
                missing.pop();
                auto& meta = _stateset.get_data(nxt);
                if ((meta._state & SynthConfig::WINNING) ||
                    (_is_safety && (meta._state & (SynthConfig::UNKNOWN | SynthConfig::LOSING)) == 0)) {
                    _stateset.decode(parent, nxt);
                    generator.prepare(parent);
                    while (generator.next_env(working)) {
                        auto res = _stateset.add(working);
                        auto& state = _stateset.get_data(res.second)._state;
                        if ((state & SynthConfig::PRINTED) == 0) {
                            missing.emplace(res.second);
                            state = state | SynthConfig::PRINTED;
                        }
                    }

                    bool first = true;
                    std::vector<uint32_t> winning;
                    std::set<size_t> winning_succs;
                    bool seen_win = false;
#ifndef NDEBUG
                    bool some = false;
#endif
                    while (generator.next_ctrl(working)) {
#ifndef NDEBUG
                        some = true;
#endif
                        auto res = _stateset.add(working);
                        auto state = _stateset.get_data(res.second)._state;
                        if ((state & SynthConfig::LOSING)) continue;
                        if (!_is_safety && (state & SynthConfig::WINNING) == 0) continue;
                        if ((state & SynthConfig::WINNING) && !seen_win) {
                            seen_win = true;
                            winning.clear();
                            winning_succs.clear();
                        }
                        winning.emplace_back(generator.fired());
                        winning_succs.emplace(res.second);
                    }
                    assert((winning_succs.size() > 0) == some);
                    for (auto w : winning_succs) {
                        auto& state = _stateset.get_data(w)._state;
                        if ((state & SynthConfig::PRINTED) == 0) {
                            missing.emplace(w);
                            state = state | SynthConfig::PRINTED;
                        }
                    }

                    for (auto wt : winning) {
                        if (first) {
                            if (!first_marking) out << ",\n";
                            first_marking = false;
                            out << "\"";
                            bool fp = true;
                            for (uint32_t p = 0; p < _net.numberOfPlaces(); ++p) {
                                if (parent[p] > 0) {
                                    if (!fp) out << ",";
                                    fp = false;
                                    out << _net.placeNames()[p] << ":" << parent[p];
                                }
                            }
                            out << "\":\n\t[";
                        }
                        if (!first)
                            out << ",";
                        first = false;
                        out << "\"" << _net.transitionNames()[wt] << "\"";
                    }
                    if (!first) out << "]";
                }
            }
            out << "\n}\n";
            if (&out == &std::cout)
                out << "##END STRATEGY##\n";
        }

        std::unique_ptr<Structures::Queue> make_queue(Strategy strategy)
        {
            switch (strategy) {
                case Strategy::HEUR:
                    std::cout << "Using DFS instead of BestFS for synthesis" << std::endl;
                case Strategy::DFS:
                    return std::make_unique<Structures::DFSQueue>(0);
                case Strategy::BFS:
                    return std::make_unique<Structures::BFSQueue>(0);
                case Strategy::RDFS:
                    return std::make_unique<Structures::RDFSQueue>(0);
                default:
                    std::cerr << "Unsupported Search Strategy for Synthesis" << std::endl;
                    std::exit(ErrorCode);
            }
        }

        void SimpleSynthesis::run(PQL::Condition* query, Strategy strategy, bool permissive) {
            // permissive == maximal in this case; there is a subtle difference
            // in wether you terminate the search at losing states (permissive)
            // or you saturate over the entire graph (maximal)
            // the later includes potential
            // safety/reachability given "wrong choices" on both sides

            stopwatch timer;
            timer.start();

            size_t cid;
            size_t nid;
            _working.copy(_net.initial(), _net.numberOfPlaces());

            auto& meta = get_config(_working, query, cid);
            meta._waiting = 1;

            auto queue = make_queue(strategy);
            SuccessorGenerator generator(_net);

            std::stack<SynthConfig*> back;
            queue->push(cid, nullptr, nullptr);

            // these could be preallocated; at most one successor pr transition
            std::vector<std::pair<size_t, SynthConfig*>> env_buffer;
            std::vector<std::pair<size_t, SynthConfig*>> ctrl_buffer;

            while (!meta.determined() || permissive) {
                while (!back.empty()) {
                    SynthConfig* next = back.top();
                    back.pop();
                    dependers_to_waiting(next, back);
                }

                if(queue->empty())
                    break;
                nid = queue->pop();
                auto& cconf = _stateset.get_data(nid);
                if (cconf.determined()) {
                    if (permissive && !cconf._dependers.empty())
                        back.push(&cconf);
                    continue; // handled already
                }
                // check predecessors
                bool any_undet = false;
                for (auto& p : cconf._dependers) {
                    SynthConfig* sc = p.second;
                    if (sc->determined()) continue;
                    //if(sc->_state == SynthConfig::MAYBE && !p.first && !permissive)
                    //    continue;
                    any_undet = true;
                    break;
                }
                if (!any_undet && &cconf != &meta) {
                    cconf._waiting = false;
                    cconf._dependers.clear();
                    continue;
                }

                //std::cerr << "PROCESSING [" << cconf._marking << "]" << std::endl;
                ++_result.exploredConfigurations;
                env_buffer.clear();
                ctrl_buffer.clear();

                assert(cconf._waiting == 1);
                cconf._state = SynthConfig::PROCESSED;
                assert(cconf._marking == nid);
                _stateset.decode(_parent, nid);
                generator.prepare(_parent);
                // first try all environment choices (if one is losing, everything is lost)
                bool some_env = false;
                // std::cerr << "ENV" << std::endl;
                while (generator.next_env(_working)) {
                    auto& child = get_config(_working, query, cid);
                    //std::cerr << "[" << cconf._marking << "] ";
                    //_net.print(parent.marking());
                    // std::cerr << "[" << child._marking << "] ";
                    //_net.print(working.marking());
                    some_env = true;
                    // std::cerr << "ENV[" << cconf._marking << "] -" << _net.transitionNames()[generator.fired()] << "-> [" << child._marking << "]" << std::endl;
                    if (child._state == SynthConfig::LOSING) {
                        // Environment can force a lose
                        cconf._state = SynthConfig::LOSING;
                        break;
                    } else if (child._state == SynthConfig::WINNING)
                        continue; // would never choose
                    if (&child == &cconf) {
                        if (_is_safety) continue; // would make ctrl win
                        else {
                            cconf._state = SynthConfig::LOSING; // env wins surely
                            break;
                        }
                    }
                    env_buffer.emplace_back(cid, &child);
                }

                // if determined, no need to add more, just backprop (later)
                bool some = false;
                bool some_winning = false;
                //std::cerr << "CTRL " << std::endl;
                if (!cconf.determined()) {
                    generator.reset();
                    while (generator.next_ctrl(_working)) {
                        auto& child = get_config(_working, query, cid);
                        //std::cerr << "[" << cconf._marking << "] ";
                        //_net.print(parent.marking());
                        //std::cerr << "[" << child._marking << "] ";
                        //_net.print(working.marking());
                        some = true;

                        //std::cerr << "CTRL[" << cconf._marking << "] -" << _net.transitionNames()[generator.fired()] << "-> [" << child._marking << "]" << std::endl;

                        if (&child == &cconf) {
                            if (_is_safety) { // maybe a win if safety ( no need to explore more)
                                cconf._state = SynthConfig::MAYBE;
                                if (!permissive) {
                                    ctrl_buffer.clear();
                                    if (env_buffer.empty()) {
                                        assert(cconf._state != SynthConfig::LOSING);
                                        cconf._state = SynthConfig::WINNING;
                                    }
                                    break;
                                }
                            } else { // would never choose
                                continue;
                            }
                        } else if (child._state == SynthConfig::LOSING) {
                            continue; // would never choose
                        } else if (child._state == SynthConfig::WINNING) {
                            some_winning = true;
                            // no need to search further! We are winning if env. cannot force us away
                            cconf._state = SynthConfig::MAYBE;
                            if (!permissive) {
                                ctrl_buffer.clear();
                                if (env_buffer.empty()) {
                                    assert(cconf._state != SynthConfig::LOSING);
                                    cconf._state = SynthConfig::WINNING;
                                }
                                break;
                            }
                        }
                        ctrl_buffer.emplace_back(cid, &child);
                    }
                }
                if (!cconf.determined()) {
                    if (some && !some_winning && ctrl_buffer.empty()) // we had a choice but all of them were bad. Env. can force us.
                    {
                        assert(cconf._state != SynthConfig::WINNING);
                        cconf._state = SynthConfig::LOSING;
                    } else if (!some && !some_env) {
                        // deadlock, bad if reachability, good if safety
                        assert(cconf._state != SynthConfig::WINNING);
                        if (_is_safety) {
                            cconf._state = SynthConfig::WINNING;
                        } else {
                            cconf._state = SynthConfig::LOSING;
                        }
                    } else if (env_buffer.empty() && some_winning) {
                        // reachability: env had not bad choice and ctrl had winning
                        assert(cconf._state != SynthConfig::LOSING);
                        cconf._state = SynthConfig::WINNING;
                    } else if (!some && some_env && env_buffer.empty()) {
                        // env is forced to be good.
                        cconf._state = SynthConfig::WINNING;
                    } else if (!some && !env_buffer.empty()) {
                        cconf._state = SynthConfig::MAYBE;
                    }
                }
                // if determined, no need to add to queue, just backprop
                //std::cerr << "FINISHED " << cconf._marking << " STATE " << (int)cconf._state << std::endl;
                if (cconf.determined()) {
                    //std::cerr << "DET [" << cconf._marking << "] : " << (int)cconf._state << std::endl;
                    back.push(&cconf);
                }
                if (!cconf.determined() || permissive) {

                    // we want to explore the ctrl last (assuming DFS). (TODO: check if a queue-split would be good?)
                    //std::cerr << "[" << nid << "]" << std::endl;
                    for (auto& c : ctrl_buffer) {
                        if (c.second->_waiting == 0) {
                            queue->push(c.first, nullptr, nullptr);
                            c.second->_waiting = 1;
                        }
                        c.second->_dependers.emplace_front(true, &cconf);
                    }
                    // then env.
                    for (auto& c : env_buffer) {
                        if (c.second->_waiting == 0) {
                            queue->push(c.first, nullptr, nullptr);
                            c.second->_waiting = 1;
                        }
                        c.second->_dependers.emplace_front(false, &cconf);
                    }
                    cconf._ctrl_children = ctrl_buffer.size();
                    cconf._env_children = env_buffer.size();
                    _result.numberOfEdges += cconf._ctrl_children + cconf._env_children;
                }
            }

            assert(!permissive || queue->empty());
            timer.stop();
            _result.duration = timer.duration();

            if (!_is_safety) _result.result = meta._state == SynthConfig::WINNING;
            else _result.result = meta._state != meta.LOSING;
        }
    }
}