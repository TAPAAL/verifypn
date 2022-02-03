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
#include "PetriEngine/PQL/PlaceUseVisitor.h"

namespace PetriEngine {
    namespace Synthesis {
        class GameStubbornSet : protected StubbornSet {
        public:
            GameStubbornSet(const PetriNet& net, PQL::Condition* predicate, bool is_safety);

            virtual bool prepare(const Structures::State *marking) override;
            uint32_t next_env();
            uint32_t next_ctrl();
            bool has_ctrl() const { return !_ctrl_acts.empty(); }
            bool has_env() const { return !_env_acts.empty(); }
            virtual void reset();
        protected:
            virtual void addToStub(uint32_t t);
        private:
            void skip();
            void computeSafe();
            bool approximateFuture(const bool ctrl);
            void computeBounds();
            light_deque<uint32_t> _ctrl_acts;
            light_deque<uint32_t> _env_acts;
            bool _is_safety;
            bool _added_unsafe;
            std::vector<uint32_t> _reach_actions;
            std::vector<uint32_t> _avoid_actions;
            std::vector<bool> _safe_actions;
            std::vector<bool> _safe_places;
            std::vector<bool> _inhibiting_place;
            std::vector<bool> _future_enabled;
            PQL::PlaceUseVisitor _in_query;
            std::unique_ptr<uint32_t[]> _fireing_bounds;
            std::unique_ptr<std::pair<uint32_t,uint32_t>[]> _place_bounds;
            bool _added_enabled = false;
            static constexpr uint8_t DECR = 32;
            static constexpr uint8_t INCR = 64;
            static constexpr uint8_t WAITING = 128;
        };
    }
}

#endif /* GAMESTUBBORNSET_H */

