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
        bool skip;

        Arc() :
        place(std::numeric_limits<uint32_t>::max()),
        weight(std::numeric_limits<uint32_t>::max()),
        skip(false) {
        };
    };

    struct Transition {
        std::vector<Arc> pre;
        std::vector<Arc> post;
        bool skip;

        Transition() : pre(), post(), skip(false) {
        }
    };

    struct Place {
        std::vector<uint32_t> input; // things consuming
        std::vector<uint32_t> output; // things producing
        bool skip;

        Place() : input(), output(), skip(false) {
        }
    };
}
#endif /* NETSTRUCTURES_H */

