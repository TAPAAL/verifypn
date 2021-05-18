/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *
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

#ifndef COLOREDPETRINETBUILDER_H
#define COLOREDPETRINETBUILDER_H

#include <vector>
#include <unordered_map>

#include "../AbstractPetriNetBuilder.h"
#include "../PetriNetBuilder.h"
#include "BindingGenerator.h"
#include "IntervalGenerator.h"
#include "PartitionBuilder.h"
#include "ArcIntervals.h"

namespace PetriEngine {

    class ColoredPetriNetBuilder : public AbstractPetriNetBuilder {
    public:
        typedef std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>> PTPlaceMap;
        typedef std::unordered_map<std::string, std::vector<std::string>> PTTransitionMap;
        
    public:
        ColoredPetriNetBuilder();
        ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig);
        virtual ~ColoredPetriNetBuilder();
        
        void addPlace(const std::string& name,
                int tokens,
                double x = 0,
                double y = 0) override ;
        void addPlace(const std::string& name,
                Colored::ColorType* type,
                Colored::Multiset&& tokens,
                double x = 0,
                double y = 0) override;
        void addTransition(const std::string& name,
                double x = 0,
                double y = 0) override;
        void addTransition(const std::string& name,
                const Colored::GuardExpression_ptr& guard,
                double x = 0,
                double y = 0) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                int weight = 1) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                const Colored::ArcExpression_ptr& expr) override;
        void addColorType(const std::string& id,
                Colored::ColorType* type) override;


        void sort() override;

        double getUnfoldTime() const {
            return _time;
        }

        double getPartitionTime() const {
            return _partitionTimer;
        }

        double getFixpointTime() const {
            return _fixPointCreationTime;
        }

        uint32_t getPlaceCount() const {
            return _places.size();
        }

        uint32_t getMaxIntervals() const {
            return _maxIntervals;
        }

        uint32_t getTransitionCount() const {
            return _transitions.size();
        }

        uint32_t getArcCount() const {
            uint32_t sum = 0;
            for (auto& t : _transitions) {
                sum += t.input_arcs.size();
                sum += t.output_arcs.size();
            }
            return sum;
        }

        uint32_t getUnfoldedPlaceCount() const {
            //return _nptplaces;
            return _ptBuilder.numberOfPlaces();
        }

        uint32_t getUnfoldedTransitionCount() const {
            return _ptBuilder.numberOfTransitions();
        }

        uint32_t getUnfoldedArcCount() const {
            return _nptarcs;
        }

        bool isUnfolded() const {
            return _unfolded;
        }

        const PTPlaceMap& getUnfoldedPlaceNames() const {
            return _ptplacenames;
        }

        const PTTransitionMap& getUnfoldedTransitionNames() const {
            return _pttransitionnames;
        }
        
        PetriNetBuilder& unfold();
        PetriNetBuilder& stripColors();
        void computePlaceColorFixpoint(uint32_t max_intervals, uint32_t max_intervals_reduced, int32_t timeout);
        void computePartition(int32_t timeout);
        
    private:
        std::unordered_map<std::string,uint32_t> _placenames;
        std::unordered_map<std::string,uint32_t> _transitionnames;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
        std::unordered_map<uint32_t,std::vector<uint32_t>> _placePostTransitionMap;
        std::unordered_map<uint32_t,std::vector<uint32_t>> _placePreTransitionMap;
        std::unordered_map<uint32_t,FixpointBindingGenerator> _bindings;
        PTPlaceMap _ptplacenames;
        PTTransitionMap _pttransitionnames;
        uint32_t _nptplaces = 0;
        uint32_t _npttransitions = 0;
        uint32_t _nptarcs = 0;
        uint32_t _maxIntervals = 0;
        PetriEngine::IntervalGenerator intervalGenerator;
        
        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
        ColorTypeMap _colors;
        PetriNetBuilder _ptBuilder;
        bool _unfolded = false;
        bool _stripped = false;
        bool _fixpointDone = false;
        bool _partitionComputed = false;

        std::vector<uint32_t> _placeFixpointQueue;
        std::unordered_map<uint32_t, Colored::EquivalenceVec> _partition;

        double _time;
        double _fixPointCreationTime;

        double _partitionTimer = 0;
        double _placeTime = 0;
        double _arcTime = 0;

        std::string arcToString(Colored::Arc& arc) const ;

        void printPlaceTable();

        

        std::unordered_map<uint32_t, Colored::ArcIntervals> setupTransitionVars(Colored::Transition transition);
        
        void addArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool input);

        void findStablePlaces();

        void getArcIntervals(Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId);      
        void processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals);
        void processOutputArcs(Colored::Transition& transition);
        
        void unfoldPlace(const Colored::Place* place, const PetriEngine::Colored::Color *color, uint32_t unfoldPlace, uint32_t id);
        void unfoldTransition(Colored::Transition& transition);
        void handleOrphanPlace(const Colored::Place& place, const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap);
        void createPartionVarmaps();

        void unfoldArc(const Colored::Arc& arc, const Colored::ExpressionContext::BindingMap& binding, const std::string& name);
    };
    
    //Used for checking if a variable is inside either a succ or pred expression
    enum ExpressionType {
        None,
        Pred,
        Succ
    };
 

}

#endif /* COLOREDPETRINETBUILDER_H */
