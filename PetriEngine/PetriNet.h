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

    struct TransPtr {
        uint32_t inputs;
        uint32_t outputs;
    };
    
    struct Invariant {
        uint32_t place;
        uint32_t tokens;
    };
    
    /** Type used for holding markings values */
    typedef uint32_t MarkVal;
#define MARK_INF     INT_MAX

    /** Efficient representation of PetriNet */
    class PetriNet {
        PetriNet(uint32_t transitions, uint32_t invariants, uint32_t places);
    public:
        ~PetriNet();
        
        MarkVal* makeInitialMarking();
        /** Fire transition if possible and store result in result */
        bool deadlocked(const MarkVal* marking) const;
        bool next(Structures::State* write);
        uint32_t fireing()
        {
            return _suc_tcounter -1;
        }
        void reset(const Structures::State* p);

        uint32_t numberOfTransitions() const {
            return _ntransitions;
        }

        uint32_t numberOfPlaces() const {
            return _nplaces;
        }
        int inArc(unsigned int place, unsigned int transition) const;
        int outArc(unsigned int transition, unsigned int place) const;
        
    private:
        
        void print(MarkVal const * const val) const
        {
            for(size_t i = 0; i < _nplaces; ++i)
            {
                if(val[i] != 0)
                {
                    std::cout << i << " -> " << val[i] << std::endl;
                }
            }
        }
        
        /** Number of x variables
         * @remarks We could also get this from the _places vector, but I don't see any
         * any complexity garentees for this type.
         */
        uint32_t _ninvariants, _ntransitions, _nplaces;

        std::vector<TransPtr> _transitions;
        std::vector<Invariant> _invariants;
        std::vector<uint32_t> _placeToPtrs;
        MarkVal* _initialMarking;
        
        const Structures::State* parent;
        uint32_t _suc_pcounter;
        uint32_t _suc_tcounter;
        
        friend class PetriNetBuilder;
        friend class Reducer;
    };

} // PetriEngine

#endif // PETRINET_H
