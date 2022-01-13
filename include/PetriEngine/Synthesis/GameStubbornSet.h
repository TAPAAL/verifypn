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
            using StubbornSet::StubbornSet;

            virtual bool prepare(const Structures::State *marking) override;
        };
    }
}

#endif /* GAMESTUBBORNSET_H */

