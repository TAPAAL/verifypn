/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef ABSTRACTPETRINETBUILDER_H
#define ABSTRACTPETRINETBUILDER_H

#include <string>

#include "PQL/PQL.h"
#include "Colored/Multiset.h"
#include "Colored/Expressions.h"

namespace PetriEngine {

    /** Abstract builder for petri nets */
    class AbstractPetriNetBuilder {
    protected:
        bool _isColored = false;
        
    public:
        /** Add a new place with a unique name */
        virtual void addPlace(const std::string& name,
                int tokens,
                double x = 0,
                double y = 0) = 0;
        /** Add a new colored place with a unique name */
        virtual void addPlace(const std::string& name,
                Colored::ColorType* type,
                Colored::Multiset tokens,
                double x = 0,
                double y = 0)
        {
            std::cout << "Colored places are not supported in standard P/T nets" << std::endl;
            exit(-1);
        }
        /** Add a new transition with a unique name */
        virtual void addTransition(const std::string& name,
                double x = 0,
                double y = 0) = 0;
        /** Add a new colored transition with a unique name */
        virtual void addTransition(const std::string& name,
                Colored::GuardExpression* guard,
                double x = 0,
                double y = 0)
        {
            std::cout << "Colored transitions are not supported in standard P/T nets" << std::endl;
            exit(-1);
        }
        /** Add input arc with given weight */
        virtual void addInputArc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int) = 0;
        /** Add colored input arc with given arc expression */
        virtual void addInputArc(const std::string& place,
                const std::string& transition,
                Colored::ArcExpression* expr)
        {
            std::cout << "Colored input arcs are not supported in standard P/T nets" << std::endl;
            exit(-1);
        }
        /** Add output arc with given weight */
        virtual void addOutputArc(const std::string& transition,
                const std::string& place,
                int weight = 1) = 0;
        /** Add output arc with given arc expression */
        virtual void addOutputArc(const std::string& transition,
                const std::string& place,
                Colored::ArcExpression* expr)
        {
            std::cout << "Colored output arcs are not supported in standard P/T nets" << std::endl;
            exit(-1);
        }
        /** Add color types with id */
        virtual void addColorType(const std::string& id,
                Colored::ColorType* type)
        {
            std::cout << "Color types are not supported in standard P/T nets" << std::endl;
            exit(-1);
        }
        
        virtual void enableColors() {
            _isColored = true;
        }

        virtual bool isColored() const {
            return _isColored;
        }

        virtual void sort() = 0;
        
        virtual ~AbstractPetriNetBuilder() {
        }
    };

}

#endif // ABSTRACTPETRINETBUILDER_H
