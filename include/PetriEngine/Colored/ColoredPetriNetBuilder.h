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
#include <istream>

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
                uint32_t tokens,
                double x,
                double y) override ;
        void addPlace(const std::string& name,
                const Colored::ColorType* type,
                Colored::Multiset&& tokens,
                double x,
                double y) override;
        void addTransition(const std::string& name,
                int32_t player,
                double x,
                double y) override;
        void addTransition(const std::string& name,
                const Colored::GuardExpression_ptr& guard,
                int32_t player,
                double x,
                double y) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool inhibitor, int weight) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                int weight) override;
        void addOutputArc(const std::string& transition,
                const std::string& place,
                const Colored::ArcExpression_ptr& expr) override;
        void addColorType(const std::string& id,
                const Colored::ColorType* type) override;


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
        void computeSymmetricVariables();
        void printSymmetricVariables() const;

    private:
        std::unordered_map<std::string,uint32_t> _placenames;
        std::unordered_map<std::string,uint32_t> _transitionnames;
        std::vector<std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
        PTPlaceMap _ptplacenames;
        PTTransitionMap _pttransitionnames;
        uint32_t _nptarcs = 0;
        uint32_t _maxIntervals = 0;
        const Colored::IntervalGenerator intervalGenerator = Colored::IntervalGenerator();

        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::Arc> _inhibitorArcs;
        std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
        //transition id to vector of vectors of variables, where variable in vector are symmetric
        std::vector<std::vector<std::set<const Colored::Variable *>>> symmetric_var_map;

        std::vector<std::string> _sumPlacesNames;
        Colored::ColorTypeMap _colors;
        PetriNetBuilder _ptBuilder;
        bool _unfolded = false;
        bool _stripped = false;
        bool _fixpointDone = false;
        bool _partitionComputed = false;

        std::vector<uint32_t> _placeFixpointQueue;
        std::vector<Colored::EquivalenceVec> _partition;

        double _time;
        double _fixPointCreationTime;

        double _partitionTimer = 0;

        std::string arcToString(const Colored::Arc& arc) const ;

        void printPlaceTable() const;

        void checkSymmetricVarsInArcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible ) const;
        void checkSymmetricVarsOutArcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const;
        void removeInvalidVarmaps(Colored::Transition& transition) const;
        void addTransitionVars(Colored::Transition& transition) const;

        std::unordered_map<uint32_t, Colored::ArcIntervals> setupTransitionVars(const Colored::Transition &transition) const;

        void addArc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool input, bool inhibitor, int weight);

        void findStablePlaces();

        void getArcIntervals(const Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId);
        void processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals);
        void processOutputArcs(Colored::Transition& transition);

        void unfoldPlace(const Colored::Place* place, const PetriEngine::Colored::Color *color, uint32_t unfoldPlace, uint32_t id);
        void unfoldTransition(uint32_t transitionId);
        void handleOrphanPlace(const Colored::Place& place, const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap);
        void createPartionVarmaps();
        void unfoldInhibitorArc(const std::string &oldname, const std::string &newname);

        void unfoldArc(const Colored::Arc& arc, const Colored::BindingMap& binding, const std::string& name);
    };

    //Used for checking if a variable is inside either a succ or pred expression
    enum ExpressionType {
        None,
        Pred,
        Succ
    };


}

#endif /* COLOREDPETRINETBUILDER_H */
