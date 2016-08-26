/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#include "PNMLParser.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace PetriEngine;
using namespace XMLSP;
using namespace std;

void PNMLParser::parse(const std::string& xml,
        AbstractPetriNetBuilder* builder) {
    //Clear any left overs
    id2name.clear();
    arcs.clear();
    transitions.clear();
    transitionEnabledness.clear();

    //Set the builder
    this->builder = builder;

    //Parser the xml
    DOMElement* root = DOMElement::loadXML(xml);
    parseElement(root);

    // initialize transitionEnabledness
    for (TransitionIter it = transitions.begin(); it != transitions.end(); it++) {
        transitionEnabledness[it->id] = "(true";
    }

    //Add all the transition
    for (TransitionIter it = transitions.begin(); it != transitions.end(); it++)
        builder->addTransition(it->id, it->x, it->y);

    //Add all the arcs
    for (ArcIter it = arcs.begin(); it != arcs.end(); it++) {
        //Check that source id exists
        if (id2name.find(it->source) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc source with id=\"%s\" wasn't found!\n",
                    it->source.c_str());
            continue;
        }
        //Check that target id exists
        if (id2name.find(it->target) == id2name.end()) {
            fprintf(stderr,
                    "XML Parsing error: Arc target with id=\"%s\" wasn't found!\n",
                    it->target.c_str());
            continue;
        }
        //Find source and target
        NodeName source = id2name[it->source];
        NodeName target = id2name[it->target];

        if (source.isPlace && !target.isPlace) {
            builder->addInputArc(source.id, target.id, false, it->weight);

            // cout << "ARC: " << source.id << " to " << target.id << " weight " << it->weight << endl;
            char weight[sizeof (int) * 8 + 1];
            sprintf(weight, "%i", it->weight);

            string cond = " and \"" + source.id + "\" >= " + weight;
            transitionEnabledness[target.id] += cond;

        } else if (!source.isPlace && target.isPlace) {
            builder->addOutputArc(source.id, target.id, it->weight);
        } else {
            fprintf(stderr,
                    "XML Parsing error: Arc from \"%s\" to \"%s\" is neither input nor output!\n",
                    source.id.c_str(),
                    target.id.c_str());
        }
    }

    for(Arc& inhibitor : inhibarcs)
    {
        NodeName source = id2name[inhibitor.source];
        NodeName target = id2name[inhibitor.target];
        if (source.isPlace && !target.isPlace) {
            builder->addInputArc(source.id, target.id, true, inhibitor.weight);
        }
        else
        {
            fprintf(stderr,
                    "XML Parsing error: Inhibitor from \"%s\" to \"%s\" is not valid!\n",
                    source.id.c_str(),
                    target.id.c_str());
        }
    }
    
    // finalize transitionEnabledness
    for (TransitionIter it = transitions.begin(); it != transitions.end(); it++) {
        transitionEnabledness[it->id] += ")";
    }

    //Release DOM tree
    delete root;

    //Unset the builder
    this->builder = NULL;

    //Cleanup
    id2name.clear();
    arcs.clear();
    transitions.clear();
    inhibarcs.clear();
}

void PNMLParser::makePetriNet() {
    builder = NULL;
}

void PNMLParser::parseElement(DOMElement* element) {
    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        if ((*it)->getElementName() == "place") {
            parsePlace(*it);
        } else if ((*it)->getElementName() == "transition") {
            parseTransition(*it);
        } else if ((*it)->getElementName() == "arc" ||
                (*it)->getElementName() == "inputArc" ||
                (*it)->getElementName() == "outputArc") {
            parseArc(*it);
        } else if ((*it)->getElementName() == "transportArc") {
            parseTransportArc(*it);
        } else if ((*it)->getElementName() == "inhibitorArc") {
            parseArc(*it, true);
        } else if ((*it)->getElementName() == "variable") {
            std::cout << "variable not supported" << std::endl;
            exit(-1);
        } else if ((*it)->getElementName() == "queries") {
            parseQueries(*it);
        } else
            parseElement(*it);
    }
}

void PNMLParser::parseQueries(DOMElement* element) {
    string name, query;

    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        name = element->getAttribute("name");
        parseValue(element, query);
        Query q;
        q.name = name;
        q.text = query;
        this->queries.push_back(q);
    }
}

void PNMLParser::parsePlace(DOMElement* element) {
    double x = 0, y = 0;
    string id = element->getAttribute("id");
    int initialMarking = atoi(element->getAttribute("initialMarking").c_str());

    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        // name element is ignored
        if ((*it)->getElementName() == "graphics") {
            parsePosition(*it, x, y);
        } else if ((*it)->getElementName() == "initialMarking") {
            string text;
            parseValue(*it, text);
            initialMarking = atoi(text.c_str());
        }
    }
    //Create place
    builder->addPlace(id, initialMarking, x, y);
    //Map id to name
    NodeName nn;
    nn.id = id;
    nn.isPlace = true;
    id2name[id] = nn;
}

void PNMLParser::parseArc(DOMElement* element, bool inhibitor) {
    string source = element->getAttribute("source"),
            target = element->getAttribute("target");
    int weight = 1;

    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        if ((*it)->getElementName() == "inscription") {
            string text;
            parseValue(*it, text);
            weight = atoi(text.c_str());
        }
    }
    Arc arc;
    arc.source = source;
    arc.target = target;
    arc.weight = weight;
    if(inhibitor)
    {
        inhibarcs.push_back(arc);   
    }
    else
    {
        arcs.push_back(arc);
    }
}

void PNMLParser::parseTransportArc(DOMElement* element){
    string source	= element->getAttribute("source"),
           transiton	= element->getAttribute("transition"),
           target	= element->getAttribute("target");
    int weight = 1;

    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for(it = elements.begin(); it != elements.end(); it++){
            if((*it)->getElementName() == "inscription"){
                    string text;
                    parseValue(*it, text);
                    weight = atoi(text.c_str());
            }
    }

    Arc inArc;
    inArc.source = source;
    inArc.target = transiton;
    inArc.weight = weight;
    arcs.push_back(inArc);

    Arc outArc;
    outArc.source = transiton;
    outArc.target = target;
    outArc.weight = weight;
    arcs.push_back(outArc);
}

void PNMLParser::parseTransition(DOMElement* element) {
    Transition t;
    t.x = 0;
    t.y = 0;
    t.id = element->getAttribute("id");

    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        // name element is ignored
        if ((*it)->getElementName() == "graphics") {
            parsePosition(*it, t.x, t.y);
        } else if ((*it)->getElementName() == "conditions") {
            std::cout << "conditions not supported" << std::endl;
            exit(-1);
        } else if ((*it)->getElementName() == "assignments") {
            std::cout << "assignments not supported" << std::endl;
            exit(-1);
        }
    }
    //Add transition to list
    transitions.push_back(t);
    //Map id to name
    NodeName nn;
    nn.id = t.id;
    nn.isPlace = false;
    id2name[t.id] = nn;
}

void PNMLParser::parseValue(DOMElement* element, string& text) {
    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        if ((*it)->getElementName() == "value" || (*it)->getElementName() == "text") {
            text = (*it)->getCData();
        } else
            parseValue(*it, text);
    }
}

void PNMLParser::parsePosition(DOMElement* element, double& x, double& y) {
    DOMElements elements = element->getChilds();
    DOMElements::iterator it;
    for (it = elements.begin(); it != elements.end(); it++) {
        if ((*it)->getElementName() == "position") {
            x = atof((*it)->getAttribute("x").c_str());
            y = atof((*it)->getAttribute("y").c_str());
        } else
            parsePosition(*it, x, y);
    }
}
