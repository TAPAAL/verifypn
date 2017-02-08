#include <string.h>
#include <sstream>
#include <iostream>

#include "Marking.h"

using namespace PetriEngine;

namespace PetriNets {

//    bool Marking::operator ==(const Marking& rhs) const {
//        for(int i = 0; i < this->length(); i++){
//            if(!(this->value()[i] == rhs.value()[i])){
//                return false;
//            }
//        }
//        return true;
//    }

//    Marking::Marking(int *buffer, int size) : m_length(size)
//    {
//        this->m_marking = (MarkVal*) malloc(sizeof(MarkVal) * size);

//        for(int i = 0; i < m_length; i++){
//            m_marking[i] = buffer[i];
//        }
//    }

    Marking::Marking(MarkVal *t_marking)
    {
        this->setMarking(t_marking);
    }

    void Marking::copyMarking(const Marking &marking, uint32_t nplaces)
    {
        auto m = (MarkVal*) malloc(sizeof(MarkVal) * nplaces);
        for(int i = 0; i < nplaces; i++){
            m[i] = marking[i];
        }
        this->setMarking(m);
    }

//    std::string Marking::toString() const
//    {
//        std::stringstream ss;
//        ss << "Marking (" << this << "): ";
//        for(int i = 0; i < m_length; i++)
//            ss << m_marking[i];

//        return ss.str();
//    }

//    void Marking::print() const {
//        std::cout << toString() << std::endl;
//    }
}

