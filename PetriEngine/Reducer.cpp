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
    
void Reducer::CreateInhibitorPlacesAndTransitions(PetriNet* net, PNMLParser::InhibitorArcList inhibarcs, MarkVal* placeInInhib, MarkVal* transitionInInhib){
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
		size_t place = -1;
                //Find place number
		for(size_t i = 0; i < net->numberOfPlaces(); i++){
			if(net->placeNames()[i] == arcIter->source){
                                place=i;
                                placeInInhib[place] = arcIter->weight;
				break;
			}
		}
                
                size_t transition = -1;
                //Find place number
		for(size_t i = 0; i < net->numberOfTransitions(); i++){
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
                fprintf(stdout,"Transition %d:\n",j); 
                for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->inArc(i,j)>0) fprintf(stdout,"   Input place %d with arc-weight %d\n",i,net->inArc(i,j));                       
                }
                for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->outArc(j,i)>0) fprintf(stdout,"  Output place %d with arc-weight %d\n",i,net->outArc(j,i));                       
                }
                fprintf(stdout,"\n",j);   
            }
            
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Marking at place %d is: %d\n",i,m0[i]);
            } 
    
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Query count for place %d is: %d\n",i,placeInQuery[i]);
            } 
           
            for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Inhibitor count for place %d is: %d\n",i,placeInInhib[i]);
            } 
            
            for (int i=0; i < net->numberOfTransitions(); i++) {
                fprintf(stdout,"Inhibitor count for transition %d is: %d\n",i,transitionInInhib[i]);
            } 
    }
    
    
 void Reducer::Reduce(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionInInhib) {
          
     bool continueReductions=true;
     
     while (continueReductions){
         continueReductions=false; // repeat all reductions rules as long as something was reduced
         
         fprintf(stderr,"Rule A\n");
         // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places   
         for (size_t t=0; t < net->numberOfTransitions(); t++) {
                if (transitionInInhib[t]>0) { continue;} // if t has a connected inhibitor arc, it cannot be removed
                int pPre=-1;
                int pPost=-1;
                bool ok=true;
                for (size_t p=0; p < net->numberOfPlaces(); p++) {
                    if (net->inArc(p,t)>0) { 
                        if (pPre>=0 || net->inArc(p,t)>1 || placeInQuery[p]>0 || placeInInhib[p]>0) { pPre=-1; ok=false; break;}
                        pPre=p;
                    }
                    if (net->outArc(t,p)>0) {
                        if (pPost>=0 || net->outArc(t,p)>1 || placeInQuery[p]>0 || placeInInhib[p]>0) { pPost=-1; ok=false; break;}
                        pPost=p;
                    }
                }
                if (ok && pPre>=0 && pPost>=0 && pPre!=pPost && (m0[pPre]==0 || m0[pPost]==0)) {               
                    // Check that pPre goes only to t and that there is no other transition than t that gives to pPost
                    for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                        if (net->inArc(pPre,_t)>0 && _t != t) {ok=false; break; }
                        if (net->outArc(_t,pPost)>0 && _t != t ) {ok=false; break; }
                    }
                    if (ok) {
                        continueReductions=true;
                        // Remove transition t and the place that has no tokens in m0
                        fprintf(stderr,"Removing transition %i connected pre-place %i and post-place %i\n",(int)t,(int)pPre,(int)pPost);
                        net->updateinArc(pPre,t,0);
                        net->updateoutArc(t,pPost,0);
                        if (m0[pPre]==0) { // removing pPre
                                fprintf(stderr,"Removing place %i\n",(int)pPre);
                                for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                    net->updateoutArc(_t,pPost,net->outArc(_t,pPre));
                                    net->updateoutArc(_t,pPre,0);
                                }    
                        } else if (m0[pPost]==0) { // removing pPost
                        fprintf(stderr,"Removing place %i\n",(int)pPost);
                        for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                            net->updateinArc(pPre,_t,net->inArc(pPost,_t));
                            net->updateinArc(pPost,_t,0);
                        }
                        } 
                    }
                }              
         } // end of Rule A main for-loop
         
       
         fprintf(stderr,"Rule B\n");
         // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
         for (size_t p=0; p < net->numberOfPlaces(); p++) {
             if (placeInInhib[p]>0 || m0[p]>0) { continue;} // if p has inhibitor arc or nonzero initial marking it cannot be removed
             int tPre=-1;
             int tPost=-1;
             bool ok=true;
             for (size_t t=0; t < net->numberOfTransitions(); t++) {
                 if (net->outArc(t,p)>0) {
                     if (tPre>=0) {tPre=-1; ok=false; break;}
                     tPre=t;
                 }
                 if (net->inArc(p,t)>0) {
                     if (tPost>=0) {tPost=-1; ok=false; break;}
                     tPost=t;
                 }
             }
             if (ok && tPre>=0 && tPost>=0 && tPre!=tPost && (net->outArc(tPre,p)==net->inArc(p,tPost))) {
                 // Check if the output places of tPost do not have any inhibitor arcs connected
                 for (size_t _p=0; _p < net->numberOfPlaces(); _p++) {
                     if (net->outArc(tPost,_p)>0 && placeInInhib[_p]>0) {ok=false; break;}
                 }
                 // Check that tPre goes only to p and that there is no other place than p that gives to tPost 
                 for (size_t _p=0; _p < net->numberOfPlaces(); _p++) {
                     if (net->outArc(tPre,_p)>0 && _p != p) {ok=false; break; }
                     if (net->inArc(_p,tPost)>0 && _p != p ) {ok=false; break; }
                 }
                 if (ok) {
                     continueReductions=true;
                     // Remove place p
                     fprintf(stderr,"Removing place %i connected pre-transition %i and post-transition %i\n",(int)p,(int)tPre,(int)tPost);
                     net->updateoutArc(tPre,p,0);
                     net->updateinArc(p,tPost,0);
                     fprintf(stderr,"Removing transition %i\n",(int)tPost);
                     for (size_t _p=0; _p < net->numberOfPlaces(); _p++) { // remove tPost
                         net->updateoutArc(tPre,_p,net->outArc(tPost,_p));
                         net->updateoutArc(tPost,_p,0);
                     }
                 }
             }
         } // end of Rule B main for-loop
        
         fprintf(stderr,"Rule C\n");
         // Rule C - two transitions that put and take from the same places
         for (size_t t1=0; t1 < net->numberOfTransitions(); t1++) {
                for (size_t t2=0; t2 < net->numberOfTransitions(); t2++) {
                        if (t1==t2) { break;} 
                        bool ok=true;  // check whether post of t1 is the same as pre of t2
                        int noOfPlaces=0;
                        for (size_t p=0; p < net->numberOfPlaces(); p++) {
                                if (net->outArc(t1,p)!=net->inArc(p,t2)) {ok=false; break;}
                                // check that the places in between are not connected to any other transitions
                                if (net->outArc(t1,p)>0) {
                                    for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                        if (net->outArc(_t,p)>0 && _t != t1) {ok=false; break;}
                                        if (net->inArc(p,_t)>0 && _t != t2) {ok=false; break;}                                        
                                    }
                                     noOfPlaces++;
                                }
                                if (!ok) {break;}
                        }
                        if (ok && noOfPlaces>=2) { // Remove places that are in post of t1, are not in queries, are not inhibitor places and have empty initial marking, one place must be left
                           for (size_t p=0; p < net->numberOfPlaces(); p++) {
                               if (net->outArc(t1,p)>0 && placeInQuery[p]==0 && placeInInhib[p]==0 && m0[p]==0 && noOfPlaces>=2) {
                                   continueReductions=true;
                                   fprintf(stderr,"Removing place %i\n",(int)p);
                                   net->updateoutArc(t1,p,0);
                                   net->updateinArc(p,t2,0);
                                   noOfPlaces--;
                               }
                           }
                        }
                }
         }
         
         
     } // end of main while-loop   
}
    
} //PetriNet namespace
