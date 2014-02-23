/* 
 * File:   Reducer.h
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#ifndef REDUCER_H
#define	REDUCER_H

#include "PetriNet.h"
#include "PQL/Contexts.h"
#include <PetriParse/PNMLParser.h>

namespace PetriEngine{



/** Builder for building engine representations of PetriNets */
class Reducer{
            
public:
	Reducer(unsigned int noOfTransitions);
        ~Reducer();
        void CreateInhibitorPlacesAndTransitions(PetriNet* net, PNMLParser::InhibitorArcList inhibarcs, MarkVal* placeInInhib, MarkVal* transitionsInInhib);
        void Print(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionsInInhib);
        void Reduce(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionsInInhib, int enablereduction);
        
        int RemovedTransitions() const { return _removedTransitions; }
        int RemovedPlaces() const { return _removedPlaces; }
        int RuleA() const { return _ruleA; }
        int RuleB() const { return _ruleB; }
        int RuleC() const { return _ruleC; }
        int RuleD() const { return _ruleD; } 
        
private:
        int _removedTransitions;
        int _removedPlaces;
        int _ruleA, _ruleB, _ruleC, _ruleD;
        int* unfoldTransitions;
};


class QueryPlaceAnalysisContext: public PQL::AnalysisContext {
    PetriEngine::MarkVal* _placeInQuery;
public:
   
    QueryPlaceAnalysisContext(const PetriNet& net, MarkVal* placeInQuery):PQL::AnalysisContext(net) {_placeInQuery=placeInQuery;};


    ResolutionResult resolve(std::string identifier) const {
		ResolutionResult result;
                //fprintf(stderr,"ID %s\n",identifier.c_str());
       		result.offset = -1;
		result.success = false;
		for(size_t i = 0; i < _places.size(); i++){
			if(_places[i] == identifier){
				result.offset = i; 
                              	result.isPlace = true;
				result.success = true;
                                //fprintf(stderr,"In query: %i\n\n",(int)i);
                                _placeInQuery[i]++;
                                return result;
			}
		}
		for(size_t i = 0; i < _variables.size(); i++){
			if(_variables[i] == identifier){
				result.offset = i;
				result.isPlace = false;
				result.success = true;
				return result;
			}
		}
		return result;
	}
    
};


}


#endif	/* REDUCER_H */

