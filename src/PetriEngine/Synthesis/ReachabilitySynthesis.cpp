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

#include "PetriEngine/Synthesis/ReachabilitySynthesis.h"
#include "PetriEngine/Synthesis/SynthConfig.h"
#include "PetriEngine/options.h"
#include "utils/Stopwatch.h"
#include "CTL/CTLResult.h"
#include "PetriEngine/SuccessorGenerator.h"

#include <vector>


namespace PetriEngine {
    using namespace Reachability;

    namespace Synthesis {

        ReachabilitySynthesis::ReachabilitySynthesis(Reachability::ResultPrinter& printer, PetriNet& net, size_t kbound)
        : printer(printer), _net(net), _kbound(kbound) {
        }

        ReachabilitySynthesis::~ReachabilitySynthesis() {
        }

#define TRYSYNTH(X,S,Q,P)    run<SuccessorGenerator,X>(Q, P, strategy_out);

        ResultPrinter::Result ReachabilitySynthesis::synthesize(
            PQL::Condition& query,
            Strategy strategy,
            bool use_stubborn,
            bool keep_strategies,
            bool permissive,
            std::ostream* strategy_out) {
            using namespace Structures;
            CTLResult result(&query);
            if (auto* q = dynamic_cast<PQL::ControlCondition*>(&query)) {


                switch (strategy) {
                    case Strategy::HEUR:
                        std::cout << "Using DFS instead of BestFS for synthesis" << std::endl;
                        /*TRYSYNTH(HeuristicQueue, use_stubborn, result, permissive)
                        break;*/
                    case Strategy::DFS:
                        run(result, permissive, strategy_out);
                        break;
                    case Strategy::BFS:
                        run(result, permissive, strategy_out);
                        break;
                    case Strategy::RDFS:
                        run(result, permissive, strategy_out);
                        break;
                    default:
                        std::cerr << "Unsupported Search Strategy for Synthesis" << std::endl;
                        std::exit(ErrorCode);
                }
            } else {
                std::cerr << "ERROR: Unsupported query type:\n\t";
                query.toString(std::cerr);
                std::cerr << std::endl;
            }

            //printer.printResult(result);
            return result.result ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
        }

        size_t ReachabilitySynthesis::dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety) {
            size_t processed = 0;
            //std::cerr << "BACK[" << next->_marking << "]" << std::endl;
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
            return processed;
        }

        bool ReachabilitySynthesis::check_bound(const MarkVal* marking) {
            if (_kbound > 0) {
                size_t sum = 0;
                for (size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    sum += marking[p];
                if (_kbound < sum)
                    return false;
            }
            return true;
        }

        bool ReachabilitySynthesis::eval(PQL::Condition* cond, const MarkVal* marking) {
            PQL::EvaluationContext ctx(marking, &_net);
            return cond->evaluate(ctx) == PQL::Condition::RTRUE;
            // TODO, we can use the stability in the fixpoint computation to prun the Dep-graph
        }

#ifndef NDEBUG
        std::vector<MarkVal*> markings;
#endif

        SynthConfig& ReachabilitySynthesis::get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, Structures::State& state, PQL::Condition* prop, bool is_safety, size_t& cid) {
            // TODO, we don't actually have to store winning markings here (what is fastest, checking query or looking up marking?/memory)!
            auto res = stateset.add(state);
            cid = res.second;
            SynthConfig& meta = stateset.get_data(res.second);
            {
#ifndef NDEBUG
                Structures::State tmp(new MarkVal[_net.numberOfPlaces()]);
                stateset.decode(tmp, res.second);
                std::memcmp(tmp.marking(), state.marking(), sizeof (MarkVal) * _net.numberOfPlaces());
#endif
            }

            if (res.first) {

#ifndef NDEBUG
                markings.push_back(new MarkVal[_net.numberOfPlaces()]);
                memcpy(markings.back(), state.marking(), sizeof (MarkVal) * _net.numberOfPlaces());
#endif
                meta = {SynthConfig::UNKNOWN, false, 0, 0, SynthConfig::depends_t(), res.second};
                if (!check_bound(state.marking())) {
                    meta._state = SynthConfig::LOSING;
                } else {
                    auto res = eval(prop, state.marking());
                    if (is_safety) {
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

        void ReachabilitySynthesis::print_id(size_t id) {
            std::cerr << "[" << id << "] : ";
            Structures::State s(markings[id]);
            s.print(_net, std::cerr);
            s.release();
        }


        // validating the solution of the DEP graph (reachability-query is assumed)

        void ReachabilitySynthesis::validate(PQL::Condition* query, Structures::AnnotatedStateSet<SynthConfig>& stateset, bool is_safety) {
            MarkVal* working = new MarkVal[_net.numberOfPlaces()];
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
                    assert((res != RTRUE) == is_safety);
                else if (res != is_safety) {
                    assert(conf._state == SynthConfig::WINNING);
                    continue;
                }
                SuccessorGenerator generator(_net, true, false);
                generator.prepare(markings[id]);
                bool ok = false;
                std::vector<size_t> env_maybe;
                std::vector<size_t> env_win;
                while (generator.next(working, PetriNet::ENV)) {
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
                    while (generator.next(working, PetriNet::CTRL)) {
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
            delete[] working;
        }
#endif

        void ReachabilitySynthesis::print_strategy(std::ostream& out,
            Structures::AnnotatedStateSet<SynthConfig>& stateset, SynthConfig& meta, const bool is_safety) {
            std::stack<size_t> missing;

            auto parent = _net.makeInitialMarking();
            auto working = _net.makeInitialMarking();
            {
                auto res = stateset.add(working.get());
                missing.emplace(res.second);
            }
            if (&out == &std::cout) out << "\n##BEGIN STRATEGY##\n";
            out << "{\n";
            SuccessorGenerator generator(_net, true, is_safety);
            bool first_marking = true;
            while (!missing.empty()) {
                auto nxt = missing.top();
                missing.pop();
                auto& meta = stateset.get_data(nxt);
                if ((meta._state & SynthConfig::WINNING) ||
                    (is_safety && (meta._state & (SynthConfig::UNKNOWN | SynthConfig::LOSING)) == 0)) {
                    stateset.decode(parent.get(), nxt);
                    generator.prepare(parent.get());
                    while (generator.next(working.get(), PetriNet::ENV)) {
                        auto res = stateset.add(working.get());
                        auto& state = stateset.get_data(res.second)._state;
                        if ((state & SynthConfig::PRINTED) == 0) {
                            missing.emplace(res.second);
                            state = state | SynthConfig::PRINTED;
                        }
                    }

                    bool first = true;
                    std::vector<uint32_t> winning;
                    std::set<size_t> winning_succs;
                    bool seen_win = false;
                    bool some = false;
                    while (generator.next(working.get(), PetriNet::CTRL)) {
                        some = true;
                        auto res = stateset.add(working.get());
                        auto state = stateset.get_data(res.second)._state;
                        if ((state & SynthConfig::LOSING)) continue;
                        if (!is_safety && (state & SynthConfig::WINNING) == 0) continue;
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
                        auto& state = stateset.get_data(w)._state;
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

        void ReachabilitySynthesis::run(/*PetriEngine::ResultPrinter::DGResult& result,*/ bool permissive, std::ostream* strategy_out) {
            // permissive == maximal in this case; there is a subtle difference
            // in wether you terminate the search at losing states (permissive)
            // or you saturate over the entire graph (maximal)
            // the later includes potential
            // safety/reachability given "wrong choices" on both sides
            auto query = const_cast<PetriEngine::PQL::Condition*> (result.query);
            const bool is_safety = query->isInvariant();
            if (query == nullptr) {
                std::cerr << "Body of quantifier is null" << std::endl;
                std::exit(ErrorCode);
            }
            if (query->isTemporal()) {
                std::cerr << "Body of quantifier is temporal" << std::endl;
                std::exit(ErrorCode);
            }
            /*if(is_safety)
            {
                std::cerr << "Safety synthesis is untested and unsupported" << std::endl;
                std::exit(ErrorCode);
            }*/
            stopwatch timer;
            timer.start();
            Structures::State working(_net.makeInitialMarking());
            Structures::State parent(_net.makeInitialMarking());

            Structures::AnnotatedStateSet<SynthConfig> stateset(_net, 0);

            size_t cid;
            size_t nid;

            auto& meta = get_config(stateset, working, query, is_safety, cid);
            meta._waiting = 1;

            std::make_unique<Structures::DFSQueue> queue;
            SuccessorGenerator generator(_net, true, is_safety);

            std::stack<SynthConfig*> back;
            queue.push(cid, nullptr, nullptr);

            // these could be preallocated; at most one successor pr transition
            std::vector<std::pair<size_t, SynthConfig*>> env_buffer;
            std::vector<std::pair<size_t, SynthConfig*>> ctrl_buffer;
#ifndef NDEBUG
restart:
#endif
            while (!meta.determined() || permissive) {
                while (!back.empty()) {
                    SynthConfig* next = back.top();
                    back.pop();
                    result.processedEdges += dependers_to_waiting(next, back, is_safety);
                }

                bool any = queue.pop(nid);
                if (!any)
                    break;
                auto& cconf = stateset.get_data(nid);
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
                ++result.exploredConfigurations;
                env_buffer.clear();
                ctrl_buffer.clear();

                assert(cconf._waiting == 1);
                cconf._state = SynthConfig::PROCESSED;
                assert(cconf._marking == nid);
                stateset.decode(parent.get(), nid);
                generator.prepare(parent.get());
                // first try all environment choices (if one is losing, everything is lost)
                bool some_env = false;
                while (generator.next(working.get(), PetriEngine::PetriNet::ENV)) {
                    some_env = true;
                    auto& child = get_config(stateset, working, query, is_safety, cid);
                    //std::cerr << "ENV[" << cconf._marking << "] --> [" << child._marking << "]" << std::endl;
                    if (child._state == SynthConfig::LOSING) {
                        // Environment can force a lose
                        cconf._state = SynthConfig::LOSING;
                        break;
                    } else if (child._state == SynthConfig::WINNING)
                        continue; // would never choose
                    if (&child == &cconf) {
                        if (is_safety) continue; // would make ctrl win
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
                //tsd::cerr << "CTRL " << std::endl;
                if (!cconf.determined()) {
                    while (generator.next(working.get(), PetriNet::CTRL)) {
                        some = true;
                        auto& child = get_config(stateset, working, query, is_safety, cid);
                        //std::cerr << "CTRL[" << cconf._marking << "] --> [" << child._marking << "]" << std::endl;

                        if (&child == &cconf) {
                            if (is_safety) { // maybe a win if safety ( no need to explore more)
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
                        if (is_safety) {
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
                            queue.push(c.first, nullptr, nullptr);
                            c.second->_waiting = 1;
                        }
                        c.second->_dependers.emplace_front(true, &cconf);
                    }
                    // then env.
                    for (auto& c : env_buffer) {
                        if (c.second->_waiting == 0) {
                            queue.push(c.first, nullptr, nullptr);
                            c.second->_waiting = 1;
                        }
                        c.second->_dependers.emplace_front(false, &cconf);
                    }
                    cconf._ctrl_children = ctrl_buffer.size();
                    cconf._env_children = env_buffer.size();
                    result.numberOfEdges += cconf._ctrl_children + cconf._env_children;
                }
            }
#ifndef NDEBUG
            /*permissive = true;
            for(size_t i = 0; i < stateset.size(); ++i)
            {
                auto& c = stateset.get_data(i);
                if(c.determined())
                {
                    if(c._waiting == 0)
                    {
                        back.push(&c);
                        c._waiting = 1;
                    }
                }
            }
            if(back.size() > 0)
                goto restart;*/
#endif
            assert(!permissive || queue.empty());
            //result.numberOfConfigurations = stateset.size();
            //result.numberOfMarkings = stateset.size();
            timer.stop();
            result.duration = timer.duration();
            bool res;
            if (!is_safety) res = meta._state == SynthConfig::WINNING;
            else res = meta._state != meta.LOSING;
            result.result = (res != is_safety) ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
#ifndef NDEBUG
            {
                // can only check complete solution to dep graph.
                validate(query, stateset, is_safety);
            }
#endif

            if (strategy_out != nullptr && res) {
                print_strategy(*strategy_out, stateset, meta, is_safety);
            }
        }
    }
}