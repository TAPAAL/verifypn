#ifndef NAIVEWORKLIST_CPP
#define NAIVEWORKLIST_CPP

//#include "./ExplicitColored/Algorithm/ColoredModelChecker.h"
#include "PetriEngine/ExplicitColored/Algorithms/NaiveWorklist.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
namespace ColoredLTL {
    bool NaiveWorklist::check(){
        auto gen = PetriEngine::ExplicitColored::ColoredSuccessorGenerator(_net);
        bfs(gen, _net.initial());
    }

    template<typename S>
    bool NaiveWorklist::bfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, S& state){
        auto waiting = light_deque<S>>{states};
        auto passed = light_deque<S>{states};
        if (_formula(states)){
            return true;
        }
        while (!waiting.empty()){
            auto next = waiting.pop_front();

            while(true){
                auto successors = successor_generator.next(next);
                if (successors.empty()){
                    successor_generator.reset();
                    break;
                }
                for (auto&& s : successors){
                    auto cont = false;
                    for (auto&& p : passed){
                        if (s == p){
                            cont = true;
                            break;
                        }
                    }
                    if (!cont){
                        if (_formula(next)){
                            return true;
                        }
                        passed.push_back(s);
                        waiting.push_back(s);
                    }
                }
            }
        }
        return false;
    }

    template<typename S>
    bool NaiveWorklist::dfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, S& state){
        auto waiting = std::queue<S>>{states};
        auto passed = std::queue<S>{states};
        if (_formula(states)){
            return true;
        }
        while (!waiting.empty()){
            auto next = waiting.pop();

            while(true){
                auto successors = successor_generator.next(next);
                if (successors.empty()){
                    successor_generator.reset();
                    break;
                }
                for (auto&& s : successors){
                    auto cont = false;
                    for (auto&& p : passed){
                        if (s == p){
                            cont = true;
                            break;
                        }
                    }
                    if (!cont){
                        if (_formula(next)){
                            return true;
                        }
                        passed.push(s);
                        waiting.push(s);
                    }
                }
            }
        }
        return false;
    }
}

#endif //NAIVEWORKLIST_CPP