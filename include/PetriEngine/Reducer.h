/*
 * File:   Reducer.cpp
 * Authors:
 *      Jiri Srba
 *      Jesper Adriaan van Diepen
 *      Nicolaj Østerby Jensen
 *      Masthias Mehl Sørensen
 *
 * Created on 15 February 2014, 10:50
 * Updated 7 March 2022
 */

#ifndef REDUCER_H
#define REDUCER_H

#include "PetriNet.h"
#include "PQL/Contexts.h"
#include "../PetriParse/PNMLParser.h"
#include "NetStructures.h"

#include <vector>
#include <optional>

namespace PetriEngine {

    using ArcIter = std::vector<Arc>::iterator;

    class PetriNetBuilder;

    class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
        std::vector<uint32_t> _placeInQuery;
        bool _deadlock;
    public:

        QueryPlaceAnalysisContext(const shared_name_index_map& pnames, const shared_name_index_map& tnames, const PetriNet* net)
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

        ResolutionResult resolve(const shared_const_string& identifier, bool place) override {
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
       ExpandedArc(shared_const_string place, size_t weight) : place(place), weight(weight) {}

        friend std::ostream& operator<<(std::ostream& os, ExpandedArc const & ea) {
            for(size_t i = 0; i < ea.weight; ++i)
            {
                os << "\t\t<token place=\"" << *ea.place << "\" age=\"0\"/>\n";
            }
            return os;
        }

        shared_const_string place;
        size_t weight;
   };

    class Reducer {
    public:
        Reducer(PetriNetBuilder*);
        ~Reducer();
        void Print(QueryPlaceAnalysisContext& context); // prints the net, just for debugging
        void Reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace, int timeout, bool remove_loops,
        bool all_reach, bool all_ltl, bool contains_next, std::vector<uint32_t>& reductions);

        size_t numberOfSkippedTransitions() const {
            return _skippedTransitions.size();
        }

        size_t numberOfSkippedPlaces() const {
            return _skippedPlaces;
        }

        uint32_t numberOfUnskippedTransitions();
        uint32_t numberOfUnskippedPlaces();
        int32_t removedTransitions();
        int32_t removedPlaces();

        void printStats(std::ostream& out)
        {
            out << "Removed transitions: " << removedTransitions() << "\n"
                << "Removed places: " << removedPlaces() << "\n"
                << "Applications of rule A: " << _ruleA << "\n"
                << "Applications of rule B: " << _ruleB << "\n"
                << "Applications of rule C: " << _ruleC << "\n"
                << "Applications of rule D: " << _ruleD << "\n"
                << "Applications of rule E: " << _ruleE << "\n"
                << "Applications of rule F: " << _ruleF << "\n"
                << "Applications of rule G: " << _ruleG << "\n"
                << "Applications of rule H: " << _ruleH << "\n"
                << "Applications of rule I: " << _ruleI << "\n"
                << "Applications of rule J: " << _ruleJ << "\n"
                << "Applications of rule K: " << _ruleK << "\n"
                << "Applications of rule L: " << _ruleL << "\n"
                << "Applications of rule M: " << _ruleM << "\n"
                << "Applications of rule N: " << _ruleN << "\n"
                << "Applications of rule O: " << _ruleO << "\n"
                << "Applications of rule P: " << _ruleP << "\n"
                << "Applications of rule Q: " << _ruleQ << "\n"
                << "Applications of rule R: " << _ruleR << "\n"
                << "Applications of rule S: " << _ruleS << std::endl;
        }

        void postFire(std::ostream&, const std::string& transition) const;
        void extraConsume(std::ostream&, const std::string& transition) const;
        void initFire(std::ostream&) const;

    private:
        size_t _skippedPlaces= 0;
        std::vector<uint32_t> _skippedTransitions;
        size_t _ruleA = 0, _ruleB = 0, _ruleC = 0, _ruleD = 0, _ruleE = 0, _ruleF = 0, _ruleG = 0, _ruleH = 0,
        _ruleI = 0, _ruleJ = 0, _ruleK = 0, _ruleL = 0, _ruleM = 0, _ruleN = 0, _ruleO = 0, _ruleP = 0, _ruleQ = 0, _ruleR = 0, _ruleS = 0;

        PetriNetBuilder* parent = nullptr;
        bool reconstructTrace = false;
        std::chrono::high_resolution_clock::time_point _timer;
        int _timeout = 0;

        // The reduction methods return true if they reduced something and reductions should continue with other rules
        bool ReducebyRuleA(uint32_t* placeInQuery);
        bool ReducebyRuleB(uint32_t* placeInQuery, bool remove_deadlocks, bool remove_consumers);
        bool ReducebyRuleC(uint32_t* placeInQuery);
        bool ReducebyRuleD(uint32_t* placeInQuery, bool all_reach, bool remove_loops_no_branch);
        bool ReducebyRuleEP(uint32_t* placeInQuery);
        bool ReducebyRuleI(uint32_t* placeInQuery, bool remove_consumers);
        bool ReducebyRuleF(uint32_t* placeInQuery);
        bool ReducebyRuleFNO(uint32_t* placeInQuery);
        bool ReducebyRuleG(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers);
        bool ReducebyRuleH(uint32_t* placeInQuery, bool all_ltl);
        bool ReducebyRuleJ(uint32_t* placeInQuery);
        bool ReducebyRuleK(uint32_t* placeInQuery, bool remove_consumers);
        bool ReducebyRuleL(uint32_t* placeInQuery);
        bool ReducebyRuleM(uint32_t* placeInQuery);
        bool ReducebyRuleEFMNOP(uint32_t* placeInQuery);
        bool ReducebyRuleQ(uint32_t* placeInQuery);
        bool ReducebyRuleR(uint32_t* placeInQuery, uint32_t explosion_limiter);
        bool ReducebyRuleS(uint32_t *placeInQuery, bool remove_consumers, bool remove_loops, bool allReach, uint32_t explosion_limiter);

        std::optional<std::pair<std::vector<bool>, std::vector<bool>>> relevant(const uint32_t* placeInQuery, bool remove_consumers);

        bool remove_irrelevant(const uint32_t* placeInQuery, const std::vector<bool> &tseen, const std::vector<bool> &pseen);

        shared_const_string getTransitionName(uint32_t transition);
        shared_const_string getPlaceName(uint32_t place);

        PetriEngine::Transition& getTransition(uint32_t transition);
        ArcIter getOutArc(PetriEngine::Transition&, uint32_t place);
        ArcIter getInArc(uint32_t place, PetriEngine::Transition&);
        void eraseTransition(std::vector<uint32_t>&, uint32_t);
        void skipTransition(uint32_t);
        void skipPlace(uint32_t);
        void skipInArc(uint32_t, uint32_t);
        void skipOutArc(uint32_t, uint32_t);

        shared_const_string newTransName();

        bool consistent();

        bool hasTimedout() const {
            return genericTimeout(_timer, _timeout);
        }
        bool genericTimeout(std::chrono::high_resolution_clock::time_point timer, int timeout) const {
            auto end = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - timer);
            return (diff.count() >= timeout);
        }

        std::vector<shared_const_string> _initfire;
        std::unordered_map<std::string, std::vector<shared_const_string>> _postfire;
        std::unordered_map<std::string, std::vector<ExpandedArc>> _extraconsume;
        std::vector<uint8_t> _tflags;
        std::vector<uint8_t> _pflags;
        std::vector<uint32_t> _lower;
        size_t _tnameid = 0;
    };




}


#endif /* REDUCER_H */

