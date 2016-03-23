// Copyright (c) 2007, Przemyslaw Grzywacz
// All rights reserved.
// Modified to use vector instead of map so that the order of elements is preserved
// and removed some unused functionality by Jiri Srba, 2014.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Przemyslaw Grzywacz nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY Przemyslaw Grzywacz ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL Przemyslaw Grzywacz BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include <sstream>
#include <cstring>

#include "xmlsp_dom_vector.h"


namespace XMLSP {

    static std::string lastParseError;

    class DOMParser : public Parser {
    public:
        virtual ~DOMParser();

        DOMElement* getDOM() {
            return rootElement;
        }

        // parse events
        virtual bool on_tag_open(const std::string& tag_name, StringMap& attributes);
        virtual bool on_cdata(const std::string& cdata);
        virtual bool on_tag_close(const std::string& tag_name);
        virtual bool on_document_begin();
        virtual bool on_document_end();
        virtual void on_error(int errnr, int line, int col, const std::string& message);
    protected:

        std::stack<DOMElement*> elementStack;
        DOMElement* rootElement;
    };


    //===================================================================
    //===================================================================
    //===================================================================

    DOMParser::~DOMParser() {

    }


    //===================================================================

    bool DOMParser::on_tag_open(const std::string& tag_name, StringMap& attributes) {
        DOMElement *e = new DOMElement(tag_name);
        StringMap::iterator i;

        // copy attributes
        for (i = attributes.begin(); i != attributes.end(); i++)
            e->setAttribute(i->first, i->second);

        // get parent element
        if (elementStack.size()) {
            // add child to parent
            elementStack.top()->addChild(e);
        } else {
            // save root element
            rootElement = e;
        }

        // add element to stack
        elementStack.push(e);
        return true;
    }


    //===================================================================

    bool DOMParser::on_cdata(const std::string& cdata) {
        DOMElement *e = elementStack.top();
        std::string data;
        data.assign(e->getCData());
        data.append(cdata);
        e->setCData(data);
        return true;
    }


    //===================================================================

    bool DOMParser::on_tag_close(const std::string& tag_name) {
        elementStack.pop();
        return true;
    }


    //===================================================================

    bool DOMParser::on_document_begin() {
        rootElement = NULL;
        return true;
    }


    //===================================================================

    bool DOMParser::on_document_end() {
        // ?? why ?
        while (elementStack.size()) elementStack.pop();
        return true;
    }


    //===================================================================

    void DOMParser::on_error(int errnr, int line, int col, const std::string& message) {
        std::stringstream s;
        s << "Error(" << errnr << "): " << message << " on " << line << ":" << col;
        lastParseError = s.str();

        // free dom
        delete rootElement;
        while (elementStack.size()) elementStack.pop();
    }


    //===================================================================
    //===================================================================
    //===================================================================

    std::string DOMElement::getLastError() {
        return lastParseError;
    }


    //===================================================================

    DOMElement* DOMElement::loadXML(const std::string& xml) {
        DOMParser parser;
        if (parser.parse(xml)) {
            return parser.getDOM();
        } else {
            return NULL;
        }
    }



    //===================================================================

    DOMElement::DOMElement(const std::string& tag_name) {
        elementName = tag_name;
    }


    //===================================================================

    DOMElement::~DOMElement() {
        DOMElements::iterator mi;

        for (mi = childs.begin(); mi != childs.end(); mi++) {
            delete *mi;
        }
    }


    //===================================================================

    int DOMElement::hasAttribute(const std::string& name) {
        DOMAttributes::iterator i;
        for (i = attributes.begin(); i != attributes.end(); i++) {
            if (i->first == name) return true;
        }
        return false;
    }


    //===================================================================

    const std::string& DOMElement::getAttribute(const std::string& name) {
        return attributes[name];
    }


    //===================================================================

    void DOMElement::setAttribute(const std::string& name, const std::string& value) {
        attributes[name] = value;
    }


    //===================================================================

    DOMStringList DOMElement::getAttributeList() {
        DOMStringList r;
        DOMAttributes::iterator i;
        for (i = attributes.begin(); i != attributes.end(); i++)
            r.push_back(i->first);

        return r;
    }


    //===================================================================

    int DOMElement::childCount() {
        return childs.size();
    }


    //===================================================================

    DOMElements DOMElement::getElementsByAttribute(const std::string& attribute, const std::string& value) {
        DOMElements::iterator mi;
        DOMElements elements;

        for (mi = childs.begin(); mi != childs.end(); mi++) {
            if ((*mi)->hasAttribute(attribute) && (*mi)->getAttribute(attribute) == value) {
                elements.push_back(*mi);
            }
        }

        return elements;
    }


    //===================================================================

    DOMElements DOMElement::getChilds() {
        return childs;
    }


    //===================================================================

    void DOMElement::addChild(DOMElement* element) {
        childs.push_back(element);
    }


    //===================================================================


} // namespace XMLSP
