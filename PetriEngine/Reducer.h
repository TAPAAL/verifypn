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
        void Print(uint32_t* placeInQuery); // prints the net, just for debugging
        void Reduce(uint32_t* placeInQuery, int enablereduction);
        
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
        bool ReducebyRuleA(uint32_t* placeInQuery);
        bool ReducebyRuleB(uint32_t* placeInQuery);
        bool ReducebyRuleC(uint32_t* placeInQuery);
        bool ReducebyRuleD(uint32_t* placeInQuery);
        
        Transition& getTransition(uint32_t transition);
        ArcIter getOutArc(Transition&, uint32_t place);
        ArcIter getInArc(uint32_t place, Transition&);
        void eraseTransition(std::vector<uint32_t>&, uint32_t);
        void skipTransition(uint32_t);
        void skipPlace(uint32_t);
    };

    class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
        uint32_t* _placeInQuery;
    public:

        QueryPlaceAnalysisContext(const std::map<std::string, uint32_t>& places) : PQL::AnalysisContext(places) {
            _placeInQuery = new uint32_t[this->_places.size()];
            for (uint32_t i = 0; i < this->_places.size(); i++) {
                _placeInQuery[i] = 0;
            }
        };
        
        virtual ~QueryPlaceAnalysisContext()
        {
            delete[] _placeInQuery;
        }
        
        uint32_t*  getQueryPlaceCount(){
            return _placeInQuery;
        }

        ResolutionResult resolve(std::string identifier) const {
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


}


#endif /* REDUCER_H */

