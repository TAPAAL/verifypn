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

#include <tuple>
using std::get;
namespace PetriEngine {
    ColoredPetriNetBuilder::ColoredPetriNetBuilder(shared_string_set& string_set)
    : _ptBuilder(string_set), _string_set(string_set) {
    }

    ColoredPetriNetBuilder::ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig)
    : _placenames(orig._placenames), _transitionnames(orig._transitionnames),
       _places(orig._places), _transitions(orig._transitions), _ptBuilder(orig._string_set), _string_set(orig._string_set)
    {
    }

    ColoredPetriNetBuilder::~ColoredPetriNetBuilder() {
        // cleaning up colors
        for(auto& e : _colors)
            if(e.second != Colored::ColorType::dotInstance())
                delete e.second;
        for(auto& v : _variables)
            delete v;
        _colors.clear();
        _variables.clear();
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, uint32_t tokens, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addPlace(name, tokens, x, y);
        }
    }

    void ColoredPetriNetBuilder::addVariable(const PetriEngine::Colored::Variable* variable) {
        _variables.push_back(variable);
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, const Colored::ColorType* type, Colored::Multiset&& tokens, double x, double y) {
        auto tmp = std::make_shared<const_string>(name);
        tmp = *_string_set.insert(tmp).first;
        if(_placenames.count(tmp) == 0)
        {
            uint32_t next = _placenames.size();
            _places.emplace_back(Colored::Place {tmp, type, std::move(tokens), x, y});
            auto& place = _places.back();
            _placenames[tmp] = next;
            for(const auto& t : place.marking)
            {
                if(t.first->getColorType() != type)
                {
                    throw base_error("Mismatch in color-type on ", name, " expecting type ", type->getName(), " got ",
                        t.first->getColorType()->getName(), " (instance ", Colored::Color::toString(t.first), ")");
                }
            }
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, int32_t player, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addTransition(name, player, x, y);
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, int32_t player, double x, double y) {
        auto tmp = std::make_shared<const_string>(name);
        tmp = *_string_set.insert(tmp).first;
        if(_transitionnames.count(tmp) == 0)
        {
            uint32_t next = _transitionnames.size();
            _transitions.emplace_back(Colored::Transition {tmp, guard, player, x, y});
            _transitionnames[tmp] = next;
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, uint32_t weight) {
        if (!_isColored) {
            _ptBuilder.addInputArc(place, transition, inhibitor, weight);
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, uint32_t inhib_weight) {
        assert((expr == nullptr) != (inhib_weight == 0));
        addArc(place, transition, expr, true, inhib_weight);
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, uint32_t weight) {
        if (!_isColored) {
            _ptBuilder.addOutputArc(transition, place, weight);
        }
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
        addArc(place, transition, expr, false, 0);
    }

    void ColoredPetriNetBuilder::addArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, bool input, uint32_t inhib_weight) {
        auto stn = std::make_shared<const_string>(transition);
        auto spn = std::make_shared<const_string>(place);
        if(_transitionnames.count(stn) == 0)
            throw base_error("Transition '", transition, "' not found. ");
        if(_placenames.count(spn) == 0)
            throw base_error("Place '", place, "' not found. ");
        uint32_t p = _placenames[spn];
        uint32_t t = _transitionnames[stn];

        assert(t < _transitions.size());
        assert(p < _places.size());

        if (!input) assert(expr != nullptr);
        assert((expr == nullptr) != (inhib_weight == 0));

        // Modify arc if it already exists
        if (inhib_weight > 0) {
            for (PetriEngine::Colored::Arc& arc : _inhibitorArcs) {
                if (arc.place == p && arc.transition == t) {
                    arc.inhib_weight = std::min(arc.inhib_weight, inhib_weight);
                    return;
                }
            }
        }
        else if (input) {
            for (PetriEngine::Colored::Arc& arc : _transitions[t].input_arcs){
                if (arc.place == p){
                    std::vector<Colored::ArcExpression_ptr> exprsToAdd;
                    exprsToAdd.emplace_back(arc.expr);
                    exprsToAdd.emplace_back(expr);
                    arc.expr = std::make_shared<PetriEngine::Colored::AddExpression>(std::move(exprsToAdd));
                    return;
                }
            }
        }
        else {
            for (PetriEngine::Colored::Arc& arc : _transitions[t].output_arcs){
                if (arc.place == p){
                    std::vector<Colored::ArcExpression_ptr> addDuplicates;
                    addDuplicates.emplace_back(arc.expr);
                    addDuplicates.emplace_back(expr);
                    arc.expr = std::make_shared<PetriEngine::Colored::AddExpression>(std::move(addDuplicates));
                    return;
                }
            }
        }

        Colored::Arc arc;
        arc.place = p;
        arc.transition = t;
        if (inhib_weight > 0) {
            _places[p].inhibitor++;
            _transitions[t].inhibited++;
        }
        arc.expr = expr;
        arc.input = input;
        arc.inhib_weight = inhib_weight;

        if (inhib_weight > 0) {
            _inhibitorArcs.push_back(std::move(arc));
        } else if (input) {
            _transitions[t].input_arcs.push_back(std::move(arc));
            _places[p]._post.emplace_back(t);
        } else {
            _transitions[t].output_arcs.push_back(std::move(arc));
            _places[p]._pre.emplace_back(t);
        }
    }

    void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
        _colors[id] = type;
    }

    void ColoredPetriNetBuilder::addToColorType(Colored::ProductType* colorType, const Colored::ColorType* newConstituent) {
        colorType->addType(newConstituent);
    }

    void ColoredPetriNetBuilder::sort() {
        for (Colored::Place &place : _places) {
            std::sort(place._pre.begin(), place._pre.end());
            std::sort(place._post.begin(), place._post.end());
        }
        for (Colored::Transition &tran : _transitions) {
            std::sort(tran.input_arcs.begin(), tran.input_arcs.end(), Colored::ArcLessThanByPlace);
            std::sort(tran.output_arcs.begin(), tran.output_arcs.end(), Colored::ArcLessThanByPlace);
        }
        std::sort(_inhibitorArcs.begin(), _inhibitorArcs.end(), Colored::ArcLessThanByPlace);
    }
}
