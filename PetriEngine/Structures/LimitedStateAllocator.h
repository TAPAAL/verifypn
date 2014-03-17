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
#ifndef LIMITEDSTATEALLOCATOR_H
#define LIMITEDSTATEALLOCATOR_H

#include "../PetriNet.h"
#include "State.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

namespace PetriEngine {
namespace Structures {

/** State allocator that is limited on memory */
template<size_t blocksize = 100000>
class LimitedStateAllocator{
	struct Block{
		Block* parent;
		char* m;
	} __attribute__((__packed__));
public:
	LimitedStateAllocator(const PetriNet& net, int memorylimit = 0){ //  memory limit in MB
		_nPlaces = net.numberOfPlaces();
		_nVars = net.numberOfVariables();
		if(memorylimit != 0) 
			_blocklimit = ceil(memorylimit*1024.0*1024.0 / (stateSize() * blocksize));
		else
			_blocklimit = -1;
		_b = NULL;
		createNewBlock();
	}
	~LimitedStateAllocator(){
		while(_b){
			Block* nb = _b->parent;
			delete[] (char*)_b;
			_b = nb;
		}
	}
	/** Create new state */
	State* createState(){
		if(_offset == blocksize - 1){
			if(_blocklimit == 0)
				return NULL;
			createNewBlock();
		}
		char* d = (_b->m + sizeof(Block) + stateSize() * _offset);
		State* s = (State*)d;
		s->_parent = NULL;
		s->_parentTransition = 0;
		s->_transitionMultiplicity = 0;
		s->_marking = (MarkVal*)(d + sizeof(State));
		s->_valuation = (VarVal*)(d + sizeof(State) + sizeof(MarkVal) * _nPlaces);
		_offset++;
		return s;
	}
private:
	size_t stateSize(){
		return sizeof(State) + sizeof(MarkVal) * _nPlaces + sizeof(VarVal) * _nVars;
	}
	void createNewBlock(){
		if(_blocklimit > 0)
			_blocklimit--;
		size_t s = sizeof(Block) + stateSize() * blocksize;
		char* m = new char[s];
		memset(m, 0, s);
		Block* b = (Block*)m;
		b->parent = _b;
		b->m = m + sizeof(Block);
		_b = b;
		_offset = 0;
	}
	size_t _offset;
	Block* _b;
	int _nPlaces;
	int _nVars;
	int _blocklimit;
};

} // Structures
} // PetriEngine

#endif // LIMITEDSTATEALLOCATOR_H
