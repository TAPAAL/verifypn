/* 
 * File:   Reducer.cpp
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#include "Reducer.h"
#include "PetriNet.h"
#include <PetriParse/PNMLParser.h>

namespace PetriEngine{
    
    void Reducer::CreateInhibitorPlaces(PetriNet* net, PNMLParser::InhibitorArcList inhibarcs, MarkVal* placeInInhib){
        
        PNMLParser::InhibitorArcIter placeIter;
	for(placeIter = inhibarcs.begin(); placeIter != inhibarcs.end(); placeIter++){
		int place = -1;
                //Find place number
		for(int i = 0; i < net->numberOfPlaces(); i++){
			if(net->placeNames()[i] == placeIter->source){
                                place=i;
                                placeInInhib[place] = placeIter->weight;
				break;
			}
		}
                
		assert(place >= 0);
	}
        
    }
    
    void Reducer::Print(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib){
        fprintf(stdout,"Net reduction enabled.\n");
            fprintf(stdout,"Number of places: %i\n",net->numberOfPlaces());
            fprintf(stdout,"Number of transitions: %i\n\n",net->numberOfTransitions());
           
            for (int j=0; j < net->numberOfTransitions(); j++) {
                fprintf(stdout,"Transition %i:\n",j); 
                for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->inArc(i,j)>0) fprintf(stdout,"   Input place %i with arc-weight %i\n",i,net->inArc(i,j));                       
                }
                for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->outArc(j,i)>0) fprintf(stdout,"  Output place %i with arc-weight %i\n",i,net->outArc(j,i));                       
                }
                fprintf(stdout,"\n",j);   
            }
            
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Marking at place %i is: %i\n",i,m0[i]);
            } 
    
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Query count for place %i is: %i\n",i,placeInQuery[i]);
            } 
           
             for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Inhibitor count for place %i is: %i\n",i,placeInInhib[i]);
            } 
            
    }
    
}
