/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   GameStubbornSet.h
 * Author: pgj
 *
 * Created on 12 January 2022, 15.52
 */

#ifndef GAMESTUBBORNSET_H
#define GAMESTUBBORNSET_H

#include "../Stubborn/StubbornSet.h"

namespace PetriEngine {
    namespace Synthesis {
        class GameStubbornSet : public StubbornSet {
        public:
            GameStubbornSet(const PetriNet& net, PQL::Condition* predicate, bool is_safety);

            virtual bool prepare(const Structures::State *marking) override;

            bool has_ctrl() const { return _ctrl_cnt > 0; }
            bool has_env() const { return _env_cnt > 0; }
            virtual void reset();
        protected:
            virtual void addToStub(uint32_t t);
        private:
            void computeSafe();
            size_t _env_cnt = 0;
            size_t _ctrl_cnt = 0;
            bool _is_safety;
            bool _added_unsafe;
            std::vector<uint32_t> _reach_actions;
            std::vector<uint32_t> _avoid_actions;
            std::vector<bool> _safe_actions;
            std::vector<bool> _safe_places;
        };
    }
}

#endif /* GAMESTUBBORNSET_H */

