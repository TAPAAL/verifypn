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
#include <chrono>
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
        void addPlace(const std::string& name, int tokens, double x, double y) override;
        void addTransition(const std::string& name,
                double x,
                double y) override;
        void addInputArc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int weight) override;
        void addOutputArc(const std::string& transition, const std::string& place, int weight) override;

        virtual void sort() override;
        /** Make the resulting petri net, you take ownership */
        PetriNet* makePetriNet(bool reorder = true);
        /** Make the resulting initial marking, you take ownership */

        MarkVal const * initMarking()
        {
            return initialMarking.data();
        }
        
        uint32_t numberOfPlaces() const
        {
            return _placenames.size();
        }
        
        uint32_t numberOfTransitions() const
        {
            return _transitionnames.size();
        }
        
        const std::unordered_map<std::string, uint32_t>& getPlaceNames() const
        {
            return _placenames;
        }
        
        const std::unordered_map<std::string, uint32_t>& getTransitionNames() const
        {
            return _transitionnames;
        }

        void reduce(std::vector<std::shared_ptr<PQL::Condition> >& query, 
                    std::vector<Reachability::ResultPrinter::Result>& results, 
                    int reductiontype, bool reconstructTrace, const PetriNet* net, int timeout, std::vector<uint32_t>& reductions);
        
        size_t RemovedTransitions() const
        {
            return reducer.RemovedTransitions();
        }
        
        size_t RemovedPlaces() const
        {
            return reducer.RemovedPlaces();
        }

        void printStats(std::ostream& out)
        {
            reducer.printStats(out);
        }
        
        Reducer* getReducer() { return &reducer; }
        
        std::vector<std::pair<std::string, uint32_t>> orphanPlaces() const {
            std::vector<std::pair<std::string, uint32_t>> res;
            for(uint32_t p = 0; p < _places.size(); p++) {
                if(_places[p].consumers.size() == 0 && _places[p].producers.size() == 0) {
                    for(auto &n : _placenames) {
                        if(n.second == p) {
                            res.push_back(std::make_pair(n.first, initialMarking[p]));
                            break;
                        }
                    }
                }
            }
            return res;
        }

        double getReductionTime() const {
            // duration in seconds
            auto end = std::chrono::high_resolution_clock::now();
            return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
        }

        void startTimer() {
            _start = std::chrono::high_resolution_clock::now();
        }
        
    private:
        uint32_t nextPlaceId(std::vector<uint32_t>& counts,  std::vector<uint32_t>& pcounts, std::vector<uint32_t>& ids, bool reorder);
        std::chrono::high_resolution_clock::time_point _start;

    protected:
        std::unordered_map<std::string, uint32_t> _placenames;
        std::unordered_map<std::string, uint32_t> _transitionnames;
        
        std::vector<Transition> _transitions;
        std::vector<Place> _places;
        
        std::vector<MarkVal> initialMarking;
        Reducer reducer;
    };

}

#endif // PETRINETBUILDER_H

