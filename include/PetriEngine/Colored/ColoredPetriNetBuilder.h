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
//#include "Patterns.h"
#include "../AbstractPetriNetBuilder.h"
#include "../PetriNetBuilder.h"
#include "BindingGenerator.h"

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
        void computePlaceColorFixpoint(uint32_t max_intervals, int32_t timeout);
        
    private:
        std::unordered_map<std::string,uint32_t> _placenames;
        std::unordered_map<std::string,uint32_t> _transitionnames;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
        std::unordered_map<uint32_t,std::vector<uint32_t>> _placePostTransitionMap;
        std::unordered_map<uint32_t,FixpointBindingGenerator> _bindings;
        PTPlaceMap _ptplacenames;
        PTTransitionMap _pttransitionnames;
        uint32_t _nptplaces = 0;
        uint32_t _npttransitions = 0;
        uint32_t _nptarcs = 0;
        uint32_t _maxIntervals = 0;
        
        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::Arc> _arcs;
        std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
        ColorTypeMap _colors;
        PetriNetBuilder _ptBuilder;
        bool _unfolded = false;
        bool _stripped = false;
        bool _fixpointDone = false;

        std::vector<uint32_t> _placeFixpointQueue;


        double _time;
        double _fixPointCreationTime;
        double totalinputtime = 0;
        // double totalinputtime2 = 0;

        std::string arcToString(Colored::Arc& arc) const ;

        void printPlaceTable();

        std::unordered_map<uint32_t, Colored::ArcIntervals> setupTransitionVars(Colored::Transition transition);
        
        void addArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool input);

        bool getVarIntervals(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMaps, uint32_t transitionId);
       
        std::vector<Reachability::interval_t> getIntervalsFromInterval(Reachability::interval_t *interval, uint32_t varPosition, int32_t varModifier, std::vector<Colored::ColorType*> varColorTypes);
        void getArcVarIntervals(Reachability::intervalTuple_t& varIntervals, std::unordered_map<uint32_t, int32_t> modIndexMap, PetriEngine::Reachability::interval_t *interval, std::vector<Colored::ColorType*> varColorTypes);
        void processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals);
        void processOutputArcs(Colored::Transition& transition);
        
        void unfoldPlace(Colored::Place& place);
        void unfoldTransition(Colored::Transition& transition);
        bool handleOrphanPlace(Colored::Place& place);
        void unfoldTokenPlaces(Colored::Place& place);

        void unfoldArc(Colored::Arc& arc, Colored::ExpressionContext::BindingMap& binding, std::string& name, bool input);
    };
    
    //Used for checking if a variable is inside either a succ or pred expression
    enum ExpressionType {
        None,
        Pred,
        Succ
    };
 

}

#endif /* COLOREDPETRINETBUILDER_H */
