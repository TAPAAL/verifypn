/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ColoredPetriNetBuilder.h
 * Author: Klostergaard
 *
 * Created on 17. februar 2018, 16:25
 */

#ifndef COLOREDPETRINETBUILDER_H
#define COLOREDPETRINETBUILDER_H

#include <vector>
#include <unordered_map>

#include "ColoredNetStructures.h"

namespace PetriEngine {
    class ColoredPetriNetBuilder {
    public:
        ColoredPetriNetBuilder();
        ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig);
        virtual ~ColoredPetriNetBuilder();
        
//        /** Add a new place with a unique name */
//        void addPlace(const std::string& name,
//                int tokens,
//                double x = 0,
//                double y = 0) override;
//        /** Add a new transition with a unique name */
//        void addTransition(const std::string& name,
//                double x = 0,
//                double y = 0) override;
//        /** Add input arc with given weight */
//        void addInputArc(const std::string& place,
//                const std::string& transition,
//                bool inhibitor,
//                int) override;
//        /** Add output arc with given weight */
//        void addOutputArc(const std::string& transition,
//                const std::string& place,
//                int weight = 1) override;
//
//        void sort() override;
    private:
        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::Arc> _arcs;
        std::unordered_map<std::string,Colored::ColorType> _colors;

    };
}

#endif /* COLOREDPETRINETBUILDER_H */

