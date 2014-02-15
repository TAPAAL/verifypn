/* 
 * File:   Reducer.cpp
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#include "Reducer.h"
#include "PetriNet.h"

namespace PetriEngine{
 
    void Reducer::Print(PetriNet* net, MarkVal* m0){
        fprintf(stderr,"Net reduction enabled.\n");
            fprintf(stderr,"Number of places: %i\n",net->numberOfPlaces());
            fprintf(stderr,"Number of transitions: %i\n\n",net->numberOfTransitions());
           
            for (int i=0; i < net->numberOfPlaces(); i++) {
                for (int j=0; j < net->numberOfTransitions(); j++) {
                     fprintf(stderr,"Input arc from place %i to transitions %i: %i\n",i,j,net->inArc(i,j));   
                     fprintf(stderr,"Output arc from transition %i to place %i: %i\n\n",j,i,net->outArc(j,i));  
                }
            }
            
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stderr,"Marking at place %i is: %i\n",i,m0[i]);
            } 
            fprintf(stderr,"\n");
    }
    
}
