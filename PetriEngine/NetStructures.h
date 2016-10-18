/* 
 * File:   NetStructures.h
 * Author: Peter G. Jensen
 *
 * Created on 09 March 2016, 21:08
 */

#ifndef NETSTRUCTURES_H
#define NETSTRUCTURES_H

#include <limits>
#include <vector>

namespace PetriEngine {

    struct Arc {
        uint32_t place;
        uint32_t weight;
        bool skip = false;
        bool inhib = false;

        Arc() :
        place(std::numeric_limits<uint32_t>::max()),
        weight(std::numeric_limits<uint32_t>::max()),
        skip(false),
        inhib(false) {
        };
        
        bool operator < (const Arc& other)
        {
            return place < other.place;
        }
    };

    struct Transition {
        std::vector<Arc> pre;
        std::vector<Arc> post;
        bool skip = false;
        bool inhib = false;

        Transition() : pre(), post(), skip(false), inhib(false) {
        }
    };

    struct Place {
        std::vector<uint32_t> consumers; // things consuming
        std::vector<uint32_t> producers; // things producing
        bool skip = false;
        bool inhib = false;

        Place() : consumers(), producers(), skip(false), inhib(false) {
        }
    };
}
#endif /* NETSTRUCTURES_H */

