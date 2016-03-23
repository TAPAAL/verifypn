/* 
 * File:   Reducer.h
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#ifndef REDUCER_H
#define REDUCER_H

#include "PetriNet.h"
#include "PQL/Contexts.h"
#include "PetriParse/PNMLParser.h"
#include "NetStructures.h"

namespace PetriEngine {
    
    using ArcIter = std::vector<Arc>::iterator;
    
    class PetriNetBuilder;
    
    class Reducer {
    public:
        Reducer(PetriNetBuilder*);
        ~Reducer();
        void Print(size_t* placeInQuery); // prints the net, just for debugging
        void Reduce(size_t* placeInQuery, int enablereduction);
        
        int RemovedTransitions() const {
            return _removedTransitions;
        }

        int RemovedPlaces() const {
            return _removedPlaces;
        }

        int RuleA() const {
            return _ruleA;
        }

        int RuleB() const {
            return _ruleB;
        }

        int RuleC() const {
            return _ruleC;
        }

        int RuleD() const {
            return _ruleD;
        }

    private:
        int _removedTransitions;
        int _removedPlaces;
        int _ruleA, _ruleB, _ruleC, _ruleD;
        PetriNetBuilder* parent;

        // The reduction methods return true if they reduced something and reductions should continue with other rules
        bool ReducebyRuleA(size_t* placeInQuery);
        bool ReducebyRuleB(size_t* placeInQuery);
        bool ReducebyRuleC(size_t* placeInQuery);
        bool ReducebyRuleD(size_t* placeInQuery);
        
        Transition& getTransition(size_t transition);
        ArcIter getOutArc(Transition&, size_t place);
        ArcIter getInArc(size_t place, Transition&);
        void eraseTransition(std::vector<size_t>&, size_t);
        void skipTransition(size_t);
        void skipPlace(size_t);
    };

    class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
        size_t* _placeInQuery;
    public:

        QueryPlaceAnalysisContext(const std::map<std::string, size_t>& places) : PQL::AnalysisContext(places) {
            _placeInQuery = new size_t[this->_places.size()];
            for (size_t i = 0; i < this->_places.size(); i++) {
                _placeInQuery[i] = 0;
            }
        };
        
        virtual ~QueryPlaceAnalysisContext()
        {
            delete[] _placeInQuery;
        }
        
        size_t*  getQueryPlaceCount(){
            return _placeInQuery;
        }

        ResolutionResult resolve(std::string identifier) const {
            ResolutionResult result;
            result.offset = -1;
            result.success = false;
            auto it = _places.find(identifier);
            if(it != _places.end())
            {
                size_t i = it->second;
                result.offset = (int)i;
                result.isPlace = true;
                result.success = true;
                _placeInQuery[i]++;
                return result;
            }
            
            return result;
        }

    };


}


#endif /* REDUCER_H */

