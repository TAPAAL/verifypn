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
#include "../PetriParse/PNMLParser.h"
#include "NetStructures.h"

#include <vector>

namespace PetriEngine {

    using ArcIter = std::vector<Arc>::iterator;
    
    class PetriNetBuilder;
        
    class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
        std::vector<uint32_t> _placeInQuery;
        bool _deadlock;
    public:

        QueryPlaceAnalysisContext(const std::unordered_map<std::string, uint32_t>& pnames, const std::unordered_map<std::string, uint32_t>& tnames, const PetriNet* net) 
        : PQL::AnalysisContext(pnames, tnames, net) {
            _placeInQuery.resize(_placeNames.size(), 0);
            _deadlock = false;
        };
        
        virtual ~QueryPlaceAnalysisContext()
        {
        }
        
        uint32_t*  getQueryPlaceCount(){
            return _placeInQuery.data();
        }

        bool hasDeadlock() { return _deadlock; }
        
        virtual void setHasDeadlock() override {
            _deadlock = true;
        };
        
        ResolutionResult resolve(const std::string& identifier, bool place) override {
            if(!place) return PQL::AnalysisContext::resolve(identifier, false);
            ResolutionResult result;
            result.offset = -1;
            result.success = false;
            auto it = _placeNames.find(identifier);
            if(it != _placeNames.end())
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

   struct ExpandedArc
   {
       ExpandedArc(std::string place, size_t weight) : place(place), weight(weight) {}
       
        friend std::ostream& operator<<(std::ostream& os, ExpandedArc const & ea) {
            for(size_t i = 0; i < ea.weight; ++i)
            {
                os << "\t\t<token place=\"" << ea.place << "\" age=\"0\"/>\n";
            }
            return os;
        }
       
        std::string place;        
        size_t weight;
   };
    
    class Reducer {
    public:
        Reducer(PetriNetBuilder*);
        ~Reducer();
        void Print(QueryPlaceAnalysisContext& context); // prints the net, just for debugging
        void Reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace, int timeout, bool remove_loops, bool remove_consumers, bool next_safe, std::vector<uint32_t>& reductions);
        
        size_t RemovedTransitions() const {
            return _removedTransitions;
        }

        size_t RemovedPlaces() const {
            return _removedPlaces;
        }

        void printStats(std::ostream& out)
        {
            out << "Removed transitions: " << _removedTransitions << "\n"
                << "Removed places: " << _removedPlaces << "\n"
                << "Applications of rule A: " << _ruleA << "\n"
                << "Applications of rule B: " << _ruleB << "\n"
                << "Applications of rule C: " << _ruleC << "\n"
                << "Applications of rule D: " << _ruleD << "\n"
                << "Applications of rule E: " << _ruleE << "\n"
                << "Applications of rule F: " << _ruleF << "\n"
                << "Applications of rule G: " << _ruleG << "\n"
                << "Applications of rule H: " << _ruleH << "\n"
                << "Applications of rule I: " << _ruleI << "\n"
                << "Applications of rule J: " << _ruleJ << std::endl;
        }

        void postFire(std::ostream&, const std::string& transition);
        void extraConsume(std::ostream&, const std::string& transition);
        void initFire(std::ostream&);

    private:
        size_t _removedTransitions = 0;
        size_t _removedPlaces= 0;
        size_t _ruleA = 0, _ruleB = 0, _ruleC = 0, _ruleD = 0, _ruleE = 0, _ruleF = 0, _ruleG = 0, _ruleH = 0, _ruleI = 0, _ruleJ = 0;
        PetriNetBuilder* parent = nullptr;
        bool reconstructTrace = false;
        std::chrono::high_resolution_clock::time_point _timer;
        int _timeout = 0;

        // The reduction methods return true if they reduced something and reductions should continue with other rules
        bool ReducebyRuleA(uint32_t* placeInQuery);
        bool ReducebyRuleB(uint32_t* placeInQuery, bool remove_deadlocks, bool remove_consumers);
        bool ReducebyRuleC(uint32_t* placeInQuery);
        bool ReducebyRuleD(uint32_t* placeInQuery);
        bool ReducebyRuleE(uint32_t* placeInQuery);
        bool ReducebyRuleI(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers);
        bool ReducebyRuleF(uint32_t* placeInQuery);
        bool ReducebyRuleG(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers);
        bool ReducebyRuleH(uint32_t* placeInQuery);
        bool ReducebyRuleJ(uint32_t* placeInQuery);
        
        std::string getTransitionName(uint32_t transition);
        std::string getPlaceName(uint32_t place);
        
        Transition& getTransition(uint32_t transition);
        ArcIter getOutArc(Transition&, uint32_t place);
        ArcIter getInArc(uint32_t place, Transition&);
        void eraseTransition(std::vector<uint32_t>&, uint32_t);
        void skipTransition(uint32_t);
        void skipPlace(uint32_t);
        std::string newTransName();
        
        bool consistent();
        bool hasTimedout() const {
            auto end = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _timer);
            return (diff.count() >= _timeout);
        }
        
        std::vector<std::string> _initfire;
        std::unordered_map<std::string, std::vector<std::string>> _postfire;
        std::unordered_map<std::string, std::vector<ExpandedArc>> _extraconsume;
        std::vector<uint8_t> _tflags;
        std::vector<uint8_t> _pflags;
        size_t _tnameid = 0;
        std::vector<uint32_t> _skipped_trans;
    };

    


}


#endif /* REDUCER_H */

