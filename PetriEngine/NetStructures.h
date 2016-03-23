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
        size_t place;
        size_t weight;
        bool skip;

        Arc() :
        place(std::numeric_limits<size_t>::max()),
        weight(std::numeric_limits<size_t>::max()),
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
        std::vector<size_t> input; // things consuming
        std::vector<size_t> output; // things producing
        bool skip;

        Place() : input(), output(), skip(false) {
        }
    };
}
#endif /* NETSTRUCTURES_H */

