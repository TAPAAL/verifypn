/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "utils/errors.h"

#include <chrono>
#include <tuple>
using std::get;
namespace PetriEngine {
    ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
    }

    ColoredPetriNetBuilder::ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig)
    : _placenames(orig._placenames), _transitionnames(orig._transitionnames),
       _places(orig._places), _transitions(orig._transitions)
    {
    }

    ColoredPetriNetBuilder::~ColoredPetriNetBuilder() {
        // cleaning up colors
        for(auto& e : _colors)
        {
            if(e.second != Colored::ColorType::dotInstance())
                delete e.second;
        }
        _colors.clear();
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, uint32_t tokens, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addPlace(name, tokens, x, y);
        }
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double x, double y) {
        if(_placenames.count(name) == 0)
        {
            uint32_t next = _placenames.size();
            _places.emplace_back(Colored::Place {name, type, std::move(tokens), x, y});
            auto& place = _places.back();
            _placenames[name] = next;
            for(const auto& t : place.marking)
            {
                if(t.first->getColorType() != type)
                {
                    throw base_error("Mismatch in color-type on ", name, " expecting type ", type->getName(), " got ",
                        t.first->getColorType()->getName(), " (instance ", Colored::Color::toString(t.first), ")");
                }
            }

            //set up place color fix points and initialize queue
            if (!place.marking.empty()) {
                _placeFixpointQueue.emplace_back(next);
            }

            Colored::interval_vector_t placeConstraints;
            Colored::ColorFixpoint colorFixpoint = {placeConstraints, !place.marking.empty()};
            uint32_t colorCounter = 0;

            if(place.marking.size() == type->size()) {
                for(const auto& colorPair : place.marking){
                    if(colorPair.second > 0){
                        colorCounter++;
                    } else {
                        break;
                    }
                }
            }

            if(colorCounter == type->size()){
                colorFixpoint.constraints.addInterval(type->getFullInterval());
            } else {
                for (const auto& colorPair : place.marking) {
                    Colored::interval_t tokenConstraints;
                    uint32_t index = 0;
                    colorPair.first->getColorConstraints(tokenConstraints, index);

                    colorFixpoint.constraints.addInterval(tokenConstraints);
                }
            }

            _placeColorFixpoints.push_back(colorFixpoint);
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t player, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addTransition(name, player, x, y);
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t player, double x, double y) {
        if(_transitionnames.count(name) == 0)
        {
            uint32_t next = _transitionnames.size();
            _transitions.emplace_back(Colored::Transition {name, guard, player, x, y});
            _transitionnames[name] = next;
            _partition.emplace_back();
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, int weight) {
        if (!_isColored) {
            _ptBuilder.addInputArc(place, transition, inhibitor, weight);
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, bool inhibitor, int weight) {
        addArc(place, transition, expr, true, inhibitor, weight);
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, int weight) {
        if (!_isColored) {
            _ptBuilder.addOutputArc(transition, place, weight);
        }
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
        addArc(place, transition, expr, false, false, 1);
    }

    void ColoredPetriNetBuilder::addArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, bool input, bool inhibitor, int weight) {
        if(_transitionnames.count(transition) == 0)
            throw base_error("Transition '", transition, "' not found. ");
        if(_placenames.count(place) == 0)
            throw base_error("Place '", place, "' not found. ");
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        assert(t < _transitions.size());
        assert(p < _places.size());

        if(input) _places[p]._post.emplace_back(t);
        else      _places[p]._pre.emplace_back(t);

        Colored::Arc arc;
        arc.place = p;
        arc.transition = t;
        _places[p].inhibitor |= inhibitor;
        if(!inhibitor)
            assert(expr != nullptr);
        arc.expr = std::move(expr);
        arc.input = input;
        arc.weight = weight;
        if(inhibitor){
            _inhibitorArcs.push_back(std::move(arc));
        } else {
            input? _transitions[t].input_arcs.push_back(std::move(arc)): _transitions[t].output_arcs.push_back(std::move(arc));
        }

    }

    void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
        _colors[id] = type;
    }

    void ColoredPetriNetBuilder::sort() {
    }

    //------------------- Symmetric Variables --------------------//
    void ColoredPetriNetBuilder::computeSymmetricVariables(){
        if(_isColored){
            symmetric_var_map.resize(_transitions.size());
            for(uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++){
                const Colored::Transition &transition = _transitions[transitionId];
                if(transition.guard){
                    continue;
                    //the variables cannot appear on the guard
                }

                for(const auto &inArc : transition.input_arcs){
                    std::set<const Colored::Variable*> inArcVars;
                    std::vector<uint32_t> numbers;

                    //Application of symmetric variables for partitioned places is currently unhandled
                    if(_partitionComputed && !_partition[inArc.place].isDiagonal()){
                        continue;
                    }

                    //the expressions is eligible if it is an addexpression that contains only
                    //numberOfExpressions with the same number
                    bool isEligible = inArc.expr->isEligibleForSymmetry(numbers);

                    if(isEligible && numbers.size() > 1){
                        inArc.expr->getVariables(inArcVars);
                        //It cannot be symmetric with anything if there is only one variable
                        if(inArcVars.size() < 2){
                            continue;
                        }
                        //The variables may appear only on one input arc and one output arc
                        checkSymmetricVarsInArcs(transition, inArc, inArcVars, isEligible);


                        //All the variables have to appear on exactly one output arc and nowhere else
                        checkSymmetricVarsOutArcs(transition, inArcVars, isEligible);
                    }else{
                        isEligible = false;
                    }
                    if(isEligible){
                        symmetric_var_map[transitionId].emplace_back(inArcVars);
                    }
                }
            }
        }
    }

    void ColoredPetriNetBuilder::checkSymmetricVarsInArcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible ) const{
        for(auto& otherInArc : transition.input_arcs){
            if(inArc.place == otherInArc.place){
                continue;
            }
            std::set<const Colored::Variable*> otherArcVars;
            otherInArc.expr->getVariables(otherArcVars);
            for(auto* var : inArcVars){
                if(otherArcVars.find(var) != otherArcVars.end()){
                    isEligible = false;
                    break;
                }
            }
        }
    }

    void ColoredPetriNetBuilder::checkSymmetricVarsOutArcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const{
        uint32_t numArcs = 0;
        bool foundSomeVars = false;
        for(auto& outputArc : transition.output_arcs){
            bool foundArc = true;
            std::set<const Colored::Variable*> otherArcVars;
            outputArc.expr->getVariables(otherArcVars);
            for(auto* var : inArcVars){
                if(otherArcVars.find(var) == otherArcVars.end()){
                    foundArc = false;
                } else{
                    foundSomeVars = true;
                }
            }
            if(foundArc){
                //Application of symmetric variables for partitioned places is currently unhandled
                if(_partitionComputed && !_partition[outputArc.place].isDiagonal()){
                    isEligible = false;
                    break;
                }
                numArcs++;
                //All vars were present
                foundSomeVars = false;
            }
            //If some vars are present the vars are not eligible
            if(foundSomeVars){
                isEligible = false;
                break;
            }
        }

        if(numArcs != 1){
            isEligible = false;
        }
    }

    void ColoredPetriNetBuilder::printSymmetricVariables() const {
        for(uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++){
            const auto &transition = _transitions[transitionId];
            if ( symmetric_var_map[transitionId].empty()) {
                std::cout << "Transition " << transition.name << " has no symmetric variables" << std::endl;
            }else{
                std::cout << "Transition " << transition.name << " has symmetric variables: " << std::endl;
                for(const auto &set : symmetric_var_map[transitionId]){
                    std::string toPrint = "SET: ";
                    for(auto* variable : set){
                        toPrint += variable->name + ", ";
                    }
                    std::cout << toPrint << std::endl;
                }
            }
        }
    }


    //----------------------- Partitioning -----------------------//

    void ColoredPetriNetBuilder::computePartition(int32_t timeout){
        if(_isColored){
            auto partitionStart = std::chrono::high_resolution_clock::now();
            Colored::PartitionBuilder pBuilder = _fixpointDone ?
                Colored::PartitionBuilder(_transitions, _places, &_placeColorFixpoints) :
                Colored::PartitionBuilder(_transitions, _places);

            if(pBuilder.partitionNet(timeout)){
                //pBuilder.printPartion();
                _partition = pBuilder.getPartition();
                pBuilder.assignColorMap(_partition);
                _partitionComputed = true;
            }
            auto partitionEnd = std::chrono::high_resolution_clock::now();
            _partitionTimer = (std::chrono::duration_cast<std::chrono::microseconds>(partitionEnd - partitionStart).count())*0.000001;
        }
    }

    //----------------------- Color fixpoint -----------------------//

    void ColoredPetriNetBuilder::printPlaceTable() const{
        for (const auto &place: _places) {
            const auto &placeID = _placenames.find(place.name)->second;
            const auto &placeColorFixpoint = _placeColorFixpoints[placeID];
            std::cout << "Place: " << place.name << " in queue: " << placeColorFixpoint.inQueue  << " with colortype " << place.type->getName() << std::endl;

            for(const auto &fixpointPair : placeColorFixpoint.constraints) {
                std::cout << "[";
                for(const auto &range : fixpointPair._ranges) {
                    std::cout << range._lower << "-" << range._upper << ", ";
                }
                std::cout << "]"<< std::endl;
            }
            std::cout << std::endl;
        }
    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint(uint32_t maxIntervals, uint32_t maxIntervalsReduced, int32_t timeout) {
        if(_isColored){
            _arcIntervals.resize(_transitions.size());
            for(size_t t = 0; t < _transitions.size(); ++t)
                _arcIntervals[t] = setupTransitionVars(_transitions[t]);

            //Start timers for timing color fixpoint creation and max interval reduction steps
            auto start = std::chrono::high_resolution_clock::now();
            auto end = std::chrono::high_resolution_clock::now();
            auto reduceTimer = std::chrono::high_resolution_clock::now();
            while(!_placeFixpointQueue.empty()){
                //Reduce max interval once timeout passes
                if(maxIntervals > maxIntervalsReduced && timeout > 0 && std::chrono::duration_cast<std::chrono::seconds>(end - reduceTimer).count() >= timeout){
                    maxIntervals = maxIntervalsReduced;
                }

                uint32_t currentPlaceId = _placeFixpointQueue.back();
                _placeFixpointQueue.pop_back();
                _placeColorFixpoints[currentPlaceId].inQueue = false;

                for (auto transitionId : _places[currentPlaceId]._post) {
                    Colored::Transition& transition = _transitions[transitionId];
                    // Skip transitions that cannot add anything new,
                    // such as transitions with only constants on their arcs that have been processed once
                    if (transition.considered) continue;
                    bool transitionActivated = true;
                    transition.variableMaps.clear();

                    processInputArcs(transition, currentPlaceId, transitionId, transitionActivated, maxIntervals);

                    //If there were colors which activated the transitions, compute the intervals produced
                    if (transitionActivated) {
                        processOutputArcs(transition);
                    }
                }
                end = std::chrono::high_resolution_clock::now();
            }

            _fixpointDone = true;
            _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;

            //printPlaceTable();
            //_placeColorFixpoints.clear();
        }
    }

    //Create Arc interval structures for the transition
    std::unordered_map<uint32_t, Colored::ArcIntervals> ColoredPetriNetBuilder::setupTransitionVars(const Colored::Transition &transition) const{
        std::unordered_map<uint32_t, Colored::ArcIntervals> res;
        for(auto arc : transition.input_arcs){
            std::set<const Colored::Variable *> variables;
            Colored::PositionVariableMap varPositions;
            Colored::VariableModifierMap varModifiersMap;
            arc.expr->getVariables(variables, varPositions, varModifiersMap, false);

            Colored::ArcIntervals newArcInterval(&_placeColorFixpoints[arc.place], varModifiersMap);
            res[arc.place] = newArcInterval;
        }
        return res;
    }

    void ColoredPetriNetBuilder::createPartionVarmaps(){
        _arcIntervals.resize(_transitions.size());
        for(uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++){
            Colored::Transition &transition = _transitions[transitionId];
            std::set<const Colored::Variable *> variables;
            _arcIntervals[transitionId] = setupTransitionVars(transition);

            for(const auto &inArc : transition.input_arcs){
                Colored::ArcIntervals& arcInterval = _arcIntervals[transitionId][inArc.place];
                uint32_t index = 0;
                arcInterval._intervalTupleVec.clear();

                Colored::interval_vector_t intervalTuple;
                intervalTuple.addInterval(_places[inArc.place].type->getFullInterval());
                const PetriEngine::Colored::ColorFixpoint &cfp {intervalTuple};

                inArc.expr->getArcIntervals(arcInterval, cfp, index, 0);

                _partition[inArc.place].applyPartition(arcInterval);
            }

            intervalGenerator.getVarIntervals(transition.variableMaps, _arcIntervals[transitionId]);
            for(const auto &outArc : transition.output_arcs){
                outArc.expr->getVariables(variables);
            }
            if(transition.guard != nullptr){
                transition.guard->getVariables(variables);
            }
            for(auto* var : variables){
                for(auto& varmap : transition.variableMaps){
                    if(varmap.count(var) == 0){
                        Colored::interval_vector_t intervalTuple;
                        intervalTuple.addInterval(var->colorType->getFullInterval());
                        varmap[var] = intervalTuple;
                    }
                }
            }
        }
    }

    //Retrieve color intervals for the input arcs based on their places
    void ColoredPetriNetBuilder::getArcIntervals(const Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId){
        for (auto& arc : transition.input_arcs) {
            PetriEngine::Colored::ColorFixpoint& curCFP = _placeColorFixpoints[arc.place];
            curCFP.constraints.restrict(max_intervals);
            _maxIntervals = std::max(_maxIntervals, (uint32_t) curCFP.constraints.size());

            Colored::ArcIntervals& arcInterval = _arcIntervals[transitionId][arc.place];
            uint32_t index = 0;
            arcInterval._intervalTupleVec.clear();

            if(!arc.expr->getArcIntervals(arcInterval, curCFP, index, 0)){
                transitionActivated = false;
                return;
            }

            if(_partitionComputed){
                _partition[arc.place].applyPartition(arcInterval);
            }
        }

    }

    void ColoredPetriNetBuilder::addTransitionVars(Colored::Transition& transition) const{
        std::set<const Colored::Variable *> variables;
        transition.guard->getVariables(variables);
        for(auto* var : variables){
            for(auto& varmap : transition.variableMaps){
                if(varmap.count(var) == 0){
                    Colored::interval_vector_t intervalTuple;
                    intervalTuple.addInterval(var->colorType->getFullInterval());
                    varmap[var] = std::move(intervalTuple);
                }
            }
        }
    }

    void ColoredPetriNetBuilder::removeInvalidVarmaps(Colored::Transition& transition) const{
        std::vector<Colored::VariableIntervalMap> newVarmaps;
        for(auto& varMap : transition.variableMaps){
            bool validVarMap = true;
            for(auto& varPair : varMap){
                if(!varPair.second.hasValidIntervals()){
                    validVarMap = false;
                    break;
                } else {
                    varPair.second.simplify();
                }
            }
            if(validVarMap){
                newVarmaps.push_back(std::move(varMap));
            }
        }
        transition.variableMaps = std::move(newVarmaps);
    }

    //Retreive interval colors from the input arcs restricted by the transition guard
    void ColoredPetriNetBuilder::processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals) {
        getArcIntervals(transition, transitionActivated, max_intervals, transitionId);

        if(!transitionActivated){
            return;
        }
        if(intervalGenerator.getVarIntervals(transition.variableMaps, _arcIntervals[transitionId])){
            if(transition.guard != nullptr) {
                addTransitionVars(transition);
                transition.guard->restrictVars(transition.variableMaps);
                removeInvalidVarmaps(transition);

                if(transition.variableMaps.empty()){
                    //Guard restrictions removed all valid intervals
                    transitionActivated = false;
                    return;
                }
            }
        } else {
            //Retrieving variable intervals failed
            transitionActivated = false;
        }
    }

    void ColoredPetriNetBuilder::processOutputArcs(Colored::Transition& transition) {
        bool transitionHasVarOutArcs = false;
        for (const auto& arc : transition.output_arcs) {
            Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
            //used to check if colors are added to the place. The total distance between upper and
            //lower bounds should grow when more colors are added and as we cannot remove colors this
            //can be checked by summing the differences
            uint32_t colorsBefore = placeFixpoint.constraints.getContainedColors();

            std::set<const Colored::Variable *> variables;
            arc.expr->getVariables(variables);

            if (!variables.empty()) {
                transitionHasVarOutArcs = true;
            }



            //Apply partitioning to unbound outgoing variables such that
            // bindings are only created for colors used in the rest of the net
            if(_partitionComputed && !_partition[arc.place].isDiagonal()){
                for(auto* outVar : variables){
                    for(auto& varMap : transition.variableMaps){
                        if(varMap.count(outVar) == 0){
                            Colored::interval_vector_t varIntervalTuple;
                            for(const auto& EqClass : _partition[arc.place].getEquivalenceClasses()){
                                varIntervalTuple.addInterval(EqClass.intervals().back().getSingleColorInterval());
                            }
                            varMap[outVar] = std::move(varIntervalTuple);
                        }
                    }
                }
            } else {
                // Else if partitioning was not computed or diagonal
                // and there is a varaible which was not found on an input arc or in the guard,
                // we give it the full interval
                for(auto* var : variables){
                    for(auto& varmap : transition.variableMaps){
                        if(varmap.count(var) == 0){
                            Colored::interval_vector_t intervalTuple;
                            intervalTuple.addInterval(var->colorType->getFullInterval());
                            varmap[var] = std::move(intervalTuple);
                        }
                    }
                }
            }

            auto intervals = arc.expr->getOutputIntervals(transition.variableMaps);


            for(auto& intervalTuple : intervals){
                intervalTuple.simplify();
                for(auto& interval : intervalTuple){
                    placeFixpoint.constraints.addInterval(std::move(interval));
                }
            }
            placeFixpoint.constraints.simplify();

            //Check if the place should be added to the queue
            if (!placeFixpoint.inQueue) {
                uint32_t colorsAfter = placeFixpoint.constraints.getContainedColors();
                if (colorsAfter > colorsBefore) {
                    _placeFixpointQueue.push_back(arc.place);
                    placeFixpoint.inQueue = true;
                }
            }
        }
        //If there are no variables among the out arcs of a transition
        // and it has been activated, there is no reason to cosider it again
        if(!transitionHasVarOutArcs) {
            transition.considered = true;
        }
    }

    //Find places for which the marking cannot change as all input arcs are matched
    //by an output arc with an equivalent arc expression and vice versa
    void ColoredPetriNetBuilder::findStablePlaces(){
        for(uint32_t placeId = 0; placeId < _places.size(); placeId++){
            if( !_places[placeId]._post.empty() &&
                _places[placeId]._post.size() == _places[placeId]._pre.size()){

                for(auto transitionId : _places[placeId]._post){
                    bool matched = false;
                    for(auto transitionId2 : _places[placeId]._pre){
                        if(transitionId == transitionId2){
                            matched = true;
                            break;
                        }
                    }
                    if(!matched){
                        _places[placeId].stable = false;
                        break;
                    }
                    const Colored::Arc *inArc;
                    for(const auto &arc : _transitions[transitionId].input_arcs){
                        if(arc.place == placeId){
                            inArc = &arc;
                            break;
                        }
                    }
                    bool mirroredArcs = false;
                    for(auto& arc : _transitions[transitionId].output_arcs){
                        if(arc.place == placeId){

                            if(to_string(*arc.expr) == to_string(*inArc->expr)){
                                mirroredArcs = true;
                            }
                            break;
                        }
                    }
                    if(!mirroredArcs){
                        _places[placeId].stable = false;
                        break;
                    }
                }
            } else {
                _places[placeId].stable = false;
            }
        }
    }

    //----------------------- Unfolding -----------------------//

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();

            if(_fixpointDone){
                findStablePlaces();
            }

            if(!_fixpointDone && _partitionComputed){
                createPartionVarmaps();
            }

            for(uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++){
                unfoldTransition(transitionId);
            }

            const auto& unfoldedPlaceMap = _ptBuilder.getPlaceNames();
            for (auto& place : _places) {
               handleOrphanPlace(place, unfoldedPlaceMap);
            }

            _unfolded = true;
            auto end = std::chrono::high_resolution_clock::now();
            _time = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        }
        return _ptBuilder;
    }

    //Due to the way we unfold places, we only unfold palces connected to an arc (which makes sense)
    //However, in queries asking about orphan places it cannot find these, as they have not been unfolded
    //so we make a placeholder place which just has tokens equal to the number of colored tokens
    //Ideally, orphan places should just be translated to a constant in the query
    void ColoredPetriNetBuilder::handleOrphanPlace(const Colored::Place& place, const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap) {
        if(_ptplacenames.count(place.name) <= 0 && place.marking.size() > 0){
            const std::string &name = place.name + "_orphan";
            _ptBuilder.addPlace(name, place.marking.size(), place._x, place._y);
            _ptplacenames[place.name][0] = std::move(name);
        } else {
            uint32_t usedTokens = 0;
            bool any = false;
            for(const auto &unfoldedPlace : _ptplacenames[place.name]){
                any = true;
                auto unfoldedMarking = _ptBuilder.initMarking();
                usedTokens += unfoldedMarking[unfoldedPlaceMap.find(unfoldedPlace.second)->second];
            }

            if(place.marking.size() > usedTokens || !any){
                const std::string &name = place.name + "_orphan";
                _ptBuilder.addPlace(name, place.marking.size() - usedTokens, place._x, place._y);
                _ptplacenames[place.name][UINT32_MAX] = std::move(name);
            }
        }
    }

    void ColoredPetriNetBuilder::unfoldPlace(const Colored::Place* place, const PetriEngine::Colored::Color *color, uint32_t placeId, uint32_t id) {
        size_t tokenSize = 0;
        if(!_partitionComputed || _partition[placeId].isDiagonal()){
            tokenSize = place->marking[color];
        } else {
            const std::vector<const Colored::Color*>& tupleColors = color->getTupleColors();
            const size_t &tupleSize = _partition[placeId].getDiagonalTuplePositions().size();
            const uint32_t &classId = _partition[placeId].getColorEqClassMap().find(color)->second->id();
            const auto &diagonalTuplePos = _partition[placeId].getDiagonalTuplePositions();

            for(const auto &colorEqClassPair : _partition[placeId].getColorEqClassMap()){
                if(colorEqClassPair.second->id() == classId){
                    const std::vector<const Colored::Color*>& testColors = colorEqClassPair.first->getTupleColors();
                    bool match = true;
                    for(uint32_t i = 0; i < tupleSize; i++){
                        if(diagonalTuplePos[i] && tupleColors[i]->getId() != testColors[i]->getId()){
                            match = false;
                            break;
                        }
                    }
                    if(match){
                        tokenSize += place->marking[colorEqClassPair.first];
                    }
                }
            }
        }
        const std::string &name = place->name + "_" + std::to_string(color->getId());

        _ptBuilder.addPlace(name, tokenSize, place->_x, place->_y + (15 * color->getId()));
        _ptplacenames[place->name][id] = std::move(name);
    }

    void ColoredPetriNetBuilder::unfoldTransition(uint32_t transitionId) {
        double offset = 0;
        const Colored::Transition &transition = _transitions[transitionId];
        if(_fixpointDone || _partitionComputed){
            FixpointBindingGenerator gen(transition, _colors, symmetric_var_map[transitionId]);
            size_t i = 0;
            bool hasBindings = false;
            for (const auto &b : gen) {
                const std::string name = transition.name + "_" + std::to_string(i++);

                hasBindings = true;
                _ptBuilder.addTransition(name, transition._player, transition._x, transition._y + offset);
                offset += 15;

                for (auto& arc : transition.input_arcs) {
                    unfoldArc(arc, b, name);
                }
                for (auto& arc : transition.output_arcs) {
                    unfoldArc(arc, b, name);
                }

                _pttransitionnames[transition.name].push_back(std::move(name));
                unfoldInhibitorArc(transition.name, name);
            }
            if(!hasBindings){
                _pttransitionnames[transition.name] = std::vector<std::string>();
            }
        } else {
            NaiveBindingGenerator gen(transition, _colors);
            size_t i = 0;
            for (const auto &b : gen) {
                const std::string &name = transition.name + "_" + std::to_string(i++);
                _ptBuilder.addTransition(name, transition._player, transition._x, transition._y + offset);
                offset += 15;

                for (const auto& arc : transition.input_arcs) {
                    unfoldArc(arc, b, name);
                }
                for (const auto& arc : transition.output_arcs) {
                    unfoldArc(arc, b, name);
                }
                _pttransitionnames[transition.name].push_back(std::move(name));
                unfoldInhibitorArc(transition.name, name);
            }
        }
    }

    void ColoredPetriNetBuilder::unfoldInhibitorArc(const std::string &oldname, const std::string &newname) {
        for (uint32_t i = 0; i < _inhibitorArcs.size(); ++i) {
            if (_transitions[_inhibitorArcs[i].transition].name.compare(oldname) == 0) {
                const Colored::Arc &inhibArc = _inhibitorArcs[i];
                if(_sumPlacesNames.size() < inhibArc.place) _sumPlacesNames.resize(inhibArc.place + 1);
                const std::string& placeName = _sumPlacesNames[inhibArc.place];

                if(placeName.empty()){
                    const PetriEngine::Colored::Place& place = _places[inhibArc.place];
                    const std::string sumPlaceName = place.name + "Sum";
                    _ptBuilder.addPlace(sumPlaceName, place.marking.size(),place._x + 30, place._y - 30);
                    if(_ptplacenames.count(place.name) <= 0){
                        _ptplacenames[place.name][0] = sumPlaceName;
                    }
                    _sumPlacesNames[inhibArc.place] = std::move(sumPlaceName);
                }
                _ptBuilder.addInputArc(placeName, newname, true, inhibArc.weight);
            }
        }
    }

    void ColoredPetriNetBuilder::unfoldArc(const Colored::Arc& arc, const Colored::BindingMap& binding, const std::string& tName) {
        const PetriEngine::Colored::Place& place = _places[arc.place];
        //If the place is stable, the arc does not need to be unfolded.
        //This exploits the fact that since the transition is being unfolded with this binding
        //we know that this place contains the tokens to activate the transition for this binding
        //because color fixpoint allowed the binding
        if(_fixpointDone && place.stable){
            return;
        }

        const Colored::ExpressionContext &context {binding, _colors, _partition[arc.place]};
        const auto &ms = arc.expr->eval(context);
        int shadowWeight = 0;

        const Colored::Color *newColor;
        std::vector<uint32_t> tupleIds;
        for (const auto& color : ms) {
            if (color.second == 0) {
                continue;
            }

            if(!_partitionComputed || _partition[arc.place].isDiagonal()){
                newColor = color.first;
            } else {
                tupleIds.clear();
                color.first->getTupleId(tupleIds);

                _partition[arc.place].applyPartition(tupleIds);
                newColor = place.type->getColor(tupleIds);
            }

            shadowWeight += color.second;
            uint32_t id;
            if(!_partitionComputed || _partition[arc.place].isDiagonal()){
                id = newColor->getId();
            } else {
                id = _partition[arc.place].getUniqueIdForColor(newColor);
            }
            const std::string& pName = _ptplacenames[place.name][id];

            if (pName.empty()) {
                unfoldPlace(&place, newColor, arc.place, id);
            }

            if (arc.input) {
                _ptBuilder.addInputArc(pName, tName, false, color.second);
            } else {
                _ptBuilder.addOutputArc(tName, pName, color.second);
            }
            ++_nptarcs;
        }

        if(place.inhibitor){
            if(_sumPlacesNames.size() < arc.place) _sumPlacesNames.resize(arc.place + 1);
            const std::string sumPlaceName = _sumPlacesNames[arc.place];
            if(sumPlaceName.empty()){
                const std::string &newSumPlaceName = place.name + "Sum";
                _ptBuilder.addPlace(newSumPlaceName, place.marking.size(),place._x + 30, place._y - 30);
                _sumPlacesNames[arc.place] = std::move(newSumPlaceName);
            }


            if(shadowWeight > 0) {
                if (!arc.input) {
                    _ptBuilder.addOutputArc(tName, sumPlaceName, shadowWeight);
                } else {
                    _ptBuilder.addInputArc(sumPlaceName, tName, false, shadowWeight);
                }
                ++_nptarcs;
            }
        }
    }

    //----------------------- Strip Colors -----------------------//

    PetriNetBuilder& ColoredPetriNetBuilder::stripColors() {
        if (_unfolded) assert(false);
        if (_isColored && !_stripped) {
            for (auto& place : _places) {
                _ptBuilder.addPlace(place.name, place.marking.size(), place._x, place._y);
            }

            for (auto& transition : _transitions) {
                _ptBuilder.addTransition(transition.name, transition._player, transition._x, transition._y);
                for (const auto& arc : transition.input_arcs) {
                    try {
                        _ptBuilder.addInputArc(_places[arc.place].name, _transitions[arc.transition].name, false,
                                                arc.expr->weight());
                    } catch (Colored::WeightException& e) {
                        std::stringstream ss;
                        ss << "Exception on input arc: " << arcToString(arc) << std::endl;
                        ss << "In expression: " << *arc.expr << std::endl;
                        ss << e.what() << std::endl;
                        throw base_error(ss.str());
                    }
                }
                for (const auto& arc : transition.output_arcs) {
                    try {
                        _ptBuilder.addOutputArc(_transitions[arc.transition].name, _places[arc.place].name,
                                                arc.expr->weight());
                    } catch (Colored::WeightException& e) {
                        std::stringstream ss;
                        ss << "Exception on output arc: " << arcToString(arc) << std::endl;
                        ss << "In expression: " << *arc.expr << std::endl;
                        ss << e.what() << std::endl;
                        throw base_error(ss.str());
                    }
                }
                for(const auto& arc : _inhibitorArcs){
                    _ptBuilder.addInputArc(_places[arc.place].name, _transitions[arc.transition].name, true,
                                                arc.weight);
                }
            }

            _stripped = true;
            _isColored = false;
        }

        return _ptBuilder;
    }

    std::string ColoredPetriNetBuilder::arcToString(const Colored::Arc& arc) const {
        return !arc.input ? "(" + _transitions[arc.transition].name + ", " + _places[arc.place].name + ")" :
               "(" + _places[arc.place].name + ", " + _transitions[arc.transition].name + ")";
    }
}


