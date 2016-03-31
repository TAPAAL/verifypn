/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef PETRINETBUILDER_H
#define PETRINETBUILDER_H

#include <vector>
#include <string>
#include <memory>
#include "AbstractPetriNetBuilder.h"
#include "PQL/PQL.h"
#include "PetriNet.h"
#include "Reducer.h"
#include "NetStructures.h"
#include "Reachability/ReachabilityResult.h"
namespace PetriEngine {
    /** Builder for building engine representations of PetriNets */
    class PetriNetBuilder : public AbstractPetriNetBuilder {
    public:
        friend class Reducer;
        
    public:
        PetriNetBuilder();
        PetriNetBuilder(const PetriNetBuilder& other);
        void addPlace(const std::string& name, int tokens, double x, double y);
        void addTransition(const std::string& name,
                double x,
                double y);
        void addInputArc(const std::string& place,
                const std::string& transition,
                int weight);
        void addOutputArc(const std::string& transition, const std::string& place, int weight);
        /** Make the resulting petri net, you take ownership */
        PetriNet* makePetriNet(bool reorder = true);
        /** Make the resulting initial marking, you take ownership */

        MarkVal const * initMarking()
        {
            return initialMarking.data();
        }
        
        uint32_t numberOfPlaces(){
            return _placenames.size();
        }
        
        uint32_t numberOfTransitions()
        {
            return _transitionnames.size();
        }
        
        const std::map<std::string, uint32_t>& getPlaceNames()
        {
            return _placenames;
        }
        
        const std::map<std::string, uint32_t>& getTransitionNames()
        {
            return _transitionnames;
        }
        
        void reduce(std::vector<std::shared_ptr<PQL::Condition> >& query, 
                    std::vector<Reachability::ResultPrinter::Result>& results, 
                    int reductiontype);
        
        size_t RemovedTransitions()
        {
            return reducer.RemovedTransitions();
        }
        
        size_t RemovedPlaces()
        {
            return reducer.RemovedPlaces();
        }

        size_t RuleA() {
            return reducer.RuleA();
        }

        size_t RuleB() {
            return reducer.RuleB();
        }

        size_t RuleC() {
            return reducer.RuleC();
        }

        size_t RuleD() {
            return reducer.RuleD();
        }
        
    private:
        uint32_t nextPlaceId(std::vector<uint32_t>& counts, std::vector<uint32_t>& ids, bool reorder);
        
    protected:
        std::map<std::string, uint32_t> _placenames;
        std::map<std::string, uint32_t> _transitionnames;
        
        std::vector<Transition> _transitions;
        std::vector<Place> _places;
        
        std::vector<MarkVal> initialMarking;
        Reducer reducer;
    };

}

#endif // PETRINETBUILDER_H

