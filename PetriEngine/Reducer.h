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
        
    class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
        std::vector<uint32_t> _placeInQuery;
        bool _deadlock;
    public:

        QueryPlaceAnalysisContext(const std::map<std::string, uint32_t>& places) : PQL::AnalysisContext(places) {
            _placeInQuery.resize(_places.size(), 0);
            _deadlock = false;
        };
        
        virtual ~QueryPlaceAnalysisContext()
        {
        }
        
        uint32_t*  getQueryPlaceCount(){
            return _placeInQuery.data();
        }

        bool hasDeadlock() { return _deadlock; }
        
        virtual void setHasDeadlock(){
            _deadlock = true;
        };
        
        ResolutionResult resolve(std::string identifier) {
            ResolutionResult result;
            result.offset = -1;
            result.success = false;
            auto it = _places.find(identifier);
            if(it != _places.end())
            {
                uint32_t i = it->second;
                result.offset = (int)i;
                result.success = true;
                _placeInQuery[i]++;
                return result;
            }
            
            return result;
        }

    };
    
    class Reducer {
    public:
        Reducer(PetriNetBuilder*);
        ~Reducer();
        void Print(QueryPlaceAnalysisContext& context); // prints the net, just for debugging
        void Reduce(QueryPlaceAnalysisContext& context, int enablereduction);
        
        size_t RemovedTransitions() const {
            return _removedTransitions;
        }

        size_t RemovedPlaces() const {
            return _removedPlaces;
        }

        size_t RuleA() const {
            return _ruleA;
        }

        size_t RuleB() const {
            return _ruleB;
        }

        size_t RuleC() const {
            return _ruleC;
        }

        size_t RuleD() const {
            return _ruleD;
        }

    private:
        size_t _removedTransitions;
        size_t _removedPlaces;
        size_t _ruleA, _ruleB, _ruleC, _ruleD, _ruleX;
        PetriNetBuilder* parent;

        // The reduction methods return true if they reduced something and reductions should continue with other rules
        bool ReducebyRuleA(uint32_t* placeInQuery);
        bool ReducebyRuleB(uint32_t* placeInQuery);
        bool ReducebyRuleC(uint32_t* placeInQuery);
        bool ReducebyRuleD(uint32_t* placeInQuery);
        bool ReducebyRuleX(uint32_t* placeInQuery);
        
        Transition& getTransition(uint32_t transition);
        ArcIter getOutArc(Transition&, uint32_t place);
        ArcIter getInArc(uint32_t place, Transition&);
        void eraseTransition(std::vector<uint32_t>&, uint32_t);
        void skipTransition(uint32_t);
        void skipPlace(uint32_t);
    };

    


}


#endif /* REDUCER_H */

