/* 
 * File:   Reducer.h
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#ifndef REDUCER_H
#define	REDUCER_H

#include "PetriNet.h"

namespace PetriEngine{


/** Builder for building engine representations of PetriNets */
class Reducer

{
public:
	Reducer();
        void Print(PetriNet* net, MarkVal* m0);
        
private:

};

}


#endif	/* REDUCER_H */

