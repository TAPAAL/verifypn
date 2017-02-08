#ifndef MARKING_H
#define MARKING_H

#include "../../PetriEngine/Structures/State.h"
#include "assert.h"

#include <vector>

namespace PetriNets {

class PetriConfig;

//A Marking is essentially just an Array.
//However since C++ does not support
//VLA we must make use of a variable to
//said length.
//Along with that we are going make a few
//inline functions to make the class
//act/look/feel like an array.

class Marking : public PetriEngine::Structures::State
{
    typedef std::vector<PetriConfig*> config_container_type;

    public:    
    // Equality checker for containers
    // with pointers to markings
//    struct Marking_Equal_To{
//        bool operator()(const Marking* rhs, const Marking* lhs) const{
//            return (*rhs)==(*lhs);
//        }
//    };

    Marking(){}
    //this constructor creates a copy of the buffer (used for deserialization)
    Marking(int *buffer, int size);
    //this constructor assumes ownership of t_marking
    //TODO: Make a proper move constructor
    Marking(PetriEngine::MarkVal* t_marking);

    void copyMarking(const Marking& marking, uint32_t nplaces);

    //bool operator==(const Marking& rhs) const;

    inline PetriEngine::MarkVal& operator[](const int index) const {
        auto* m = const_cast<PetriEngine::MarkVal*>(this->marking());
        return m[index];
    }    
    //inline PetriEngine::MarkVal* value() const { return m_marking; }
//    std::string toString() const;
//    void print() const;
    //inline size_t length() const { return m_length; }

    config_container_type configurations;
private:
    //PetriEngine::MarkVal* m_marking;
    //size_t m_length;
};
}

namespace std {
    // Specializations of hash functions.
    // Normal
//    template<>
//    struct hash<PetriNets::Marking>{
//        size_t operator()(const PetriNets::Marking& t_marking ) const {
//            size_t hash = 0;
//            uint32_t& h1 = ((uint32_t*)&hash)[0];
//            uint32_t& h2 = ((uint32_t*)&hash)[1];
//            uint32_t cnt = 0;
//            for (size_t i = 0; i < t_marking.length(); i++)
//            {
//                if(t_marking[i])
//                {
//                    h1 ^= 1 << (i % 32);
//                    h2 ^= t_marking[i] << (cnt % 32);
//                    ++cnt;
//                }
//            }
//            return hash;
//        }
//    };
//    // Pointer to Marking
//    template<>
//    struct hash<PetriNets::Marking*>{
//        size_t operator()(const PetriNets::Marking* t_marking ) const {
//            hash<PetriNets::Marking> hasher;
//            return hasher.operator ()(*t_marking);
//        }
//    };
}
#endif // MARKING_H
