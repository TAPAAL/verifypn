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
    
void Reducer::CreateInhibitorPlaces(PetriNet* net, PNMLParser::InhibitorArcList inhibarcs, MarkVal* placeInInhib, MarkVal* transitionInInhib){
        //Initialize
        for(size_t i = 0; i < net->numberOfPlaces(); i++) {
                                placeInInhib[i] = 0;
                        }    
    
        for(size_t i = 0; i < net->numberOfTransitions(); i++) {
                                transitionInInhib[i] = 0;
                        }
        
        //Construct the inhibitor places/arcs
        PNMLParser::InhibitorArcIter arcIter;
	for(arcIter = inhibarcs.begin(); arcIter != inhibarcs.end(); arcIter++){
		int place = -1;
                //Find place number
		for(int i = 0; i < net->numberOfPlaces(); i++){
			if(net->placeNames()[i] == arcIter->source){
                                place=i;
                                placeInInhib[place] = arcIter->weight;
				break;
			}
		}
                
                int transition = -1;
                //Find place number
		for(int i = 0; i < net->numberOfTransitions(); i++){
			if(net->transitionNames()[i] == arcIter->target){
                                transition=i;
                                transitionInInhib[transition] = arcIter->weight;
				break;
			}
		}
                
		assert(place >= 0 && transition>=0);
	}
        
    }
    
    
void Reducer::Print(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionInInhib){
        fprintf(stdout,"\nNET INFO:\n");
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
            
            for (int i=0; i < net->numberOfTransitions(); i++) {
                fprintf(stdout,"Inhibitor count for transition %i is: %i\n",i,transitionInInhib[i]);
            } 
    }
    
    
 void Reducer::Reduce(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionInInhib) {
          
     bool continueReductions=true;
     
     while (continueReductions){
         continueReductions=false; // repeat all reductions rules as long as something was reduced
         
         // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places   
         for (int t=0; t < net->numberOfTransitions(); t++) {
                if (transitionInInhib[t]>0) { continue;} // if t has a connected inhibitor arc, it cannot be removed
                int pPre=-1;
                int pPost=-1;
                bool ok=true;
                for (int p=0; p < net->numberOfPlaces(); p++) {
                    if (net->inArc(p,t)>0) { 
                        if (pPre>=0 || net->inArc(p,t)>1 || placeInQuery[p]>0 || placeInInhib[p]>0) { pPre=-1; ok=false; break;}
                        pPre=p;
                    }
                    if (net->outArc(t,p)>0) {
                        if (pPost>=0 || net->outArc(t,p)>1 || placeInQuery[p]>0 || placeInInhib[p]>0) { pPost=-1; ok=false; break;}
                        pPost=p;
                    }
                }
                if (ok && (m0[pPre]==0 || m0[pPost]==0)) {
                    // Check that pPre goes only to t and that there is no other transition than t that gives to pPos a
                    for (int _t=0; _t < net->numberOfTransitions(); _t++) {
                        if (net->inArc(pPre,_t)>0 && _t != t) {ok=false; break; }
                        if (net->outArc(_t,pPost)>0 && _t != t ) {ok=false; break; }
                    }
                    if (ok) {
                        continueReductions=true;
                        // Remove transition t the place that has no tokens in m0
                        fprintf(stderr,"Removing transition %i connected pre-place %i and post-place %i\n",t,pPre,pPost);
                        net->updateinArc(pPre,t,0);
                        net->updateoutArc(t,pPost,0);
                        if (m0[pPre]>0) {std::swap<int>(pPre,pPost);}
                        fprintf(stderr,"Removing place %i\n",pPre);
                        for (int _t=0; _t < net->numberOfTransitions(); _t++) {
                            net->updateoutArc(_t,pPost,net->outArc(_t,pPre));
                            net->updateoutArc(_t,pPre,0);
                        }    
                    }
                }              
         } // end of Rule A main for-loop
         
         
         
     } // end of main while-loop   
}
    
} //PetriNet namespace
