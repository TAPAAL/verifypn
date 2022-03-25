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
    ColoredPetriNetBuilder::ColoredPetriNetBuilder(){
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

    void ColoredPetriNetBuilder::addVariable(const PetriEngine::Colored::Variable* variable) {
        _variables.push_back(variable);
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
        if(_transitionnames.count(transition) == 0)
            throw base_error("Transition '", transition, "' not found. ");
        if(_placenames.count(place) == 0)
            throw base_error("Place '", place, "' not found. ");
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        assert(t < _transitions.size());
        assert(p < _places.size());

        if(input) {
            _places[p]._post.emplace_back(t);
        }
        else {
            _places[p]._pre.emplace_back(t);
        }

        if (!input) assert(expr != nullptr);
        assert((expr == nullptr) != (inhib_weight == 0));

        Colored::Arc arc;
        arc.place = p;
        arc.transition = t;
        _places[p].inhibitor |= inhib_weight > 0;
        _transitions[t].inhibited |= inhib_weight > 0;
        arc.expr = std::move(expr);
        arc.input = input;
        arc.inhib_weight = inhib_weight;
        if(inhib_weight > 0){
            _inhibitorArcs.push_back(std::move(arc));
        } else if (input) {
            _transitions[t].input_arcs.push_back(std::move(arc));
        } else {
            _transitions[t].output_arcs.push_back(std::move(arc));
        }
    }

    void ColoredPetriNetBuilder::addColorType(const std::string& id, const Colored::ColorType* type) {
        _colors[id] = type;
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
