/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef PETRINET_H
#define PETRINET_H

#include <string>
#include <vector>
#include <climits>
#include <limits>
#include <iostream>

namespace PetriEngine {

    namespace PQL {
        class Condition;
    }

    namespace Structures {
        class State;
    }

    class PetriNetBuilder;
    class SuccessorGenerator;
    
    struct TransPtr {
        uint32_t inputs;
        uint32_t outputs;
    };
    
    struct Invariant {
        uint32_t place;
        uint32_t tokens;
        bool inhibitor;
        int8_t direction;
        // we can pack things here, but might give slowdown
    } /*__attribute__((packed))*/; 
    
    /** Type used for holding markings values */
    typedef uint32_t MarkVal;

    /** Efficient representation of PetriNet */
    class PetriNet {
        PetriNet(uint32_t transitions, uint32_t invariants, uint32_t places);
    public:
        ~PetriNet();

        uint32_t initial(size_t id) const;
        MarkVal* makeInitialMarking() const;
        /** Fire transition if possible and store result in result */
        bool deadlocked(const MarkVal* marking) const;
        bool fireable(const MarkVal* marking, int transitionIndex);
        std::pair<const Invariant*, const Invariant*> preset(uint32_t id) const;
        std::pair<const Invariant*, const Invariant*> postset(uint32_t id) const;
        uint32_t numberOfTransitions() const {
            return _ntransitions;
        }

        uint32_t numberOfPlaces() const {
            return _nplaces;
        }
        int inArc(uint32_t place, uint32_t transition) const;
        int outArc(uint32_t transition, uint32_t place) const;
        

        const std::vector<std::string>& transitionNames() const
        {
            return _transitionnames;
        }
        
        const std::vector<std::string>& placeNames() const
        {
            return _placenames;
        }
               
        void print(MarkVal const * const val) const
        {
            for(size_t i = 0; i < _nplaces; ++i)
            {
                if(val[i] != 0)
                {
                    std::cout << _placenames[i] << "(" << i << ")" << " -> " << val[i] << std::endl;
                }
            }
        }
        
        void sort();
        
        void toXML(std::ostream& out);
        
        const MarkVal* initial() const {
            return _initialMarking;
        }

    private:        

        /** Number of x variables
         * @remarks We could also get this from the _places vector, but I don't see any
         * any complexity garentees for this type.
         */
        uint32_t _ninvariants, _ntransitions, _nplaces;

        std::vector<TransPtr> _transitions;
        std::vector<Invariant> _invariants;
        std::vector<uint32_t> _placeToPtrs;
        MarkVal* _initialMarking;

        std::vector<std::string> _transitionnames;
        std::vector<std::string> _placenames;

        friend class PetriNetBuilder;
        friend class Reducer;
        friend class SuccessorGenerator;
        friend class ReducingSuccessorGenerator;
        friend class STSolver;
    };

} // PetriEngine

#endif // PETRINET_H
