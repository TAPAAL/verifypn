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
        
        bool operator < (const Arc& other) const
        {
            return place < other.place;
        }
        
        bool operator == (const Arc& other) const
        {
            return place == other.place && weight == other.weight && inhib == other.inhib;
        }
    };

    struct Transition {
        std::vector<Arc> pre;
        std::vector<Arc> post;
        bool skip = false;
        bool inhib = false;
        
        void addPreArc(const Arc& arc)
        {
            auto lb = std::lower_bound(pre.begin(), pre.end(), arc);
            if(lb != pre.end() && lb->place == arc.place)
                lb->weight += arc.weight;
            else
                lb = pre.insert(lb, arc);
            assert(lb->weight > 0);
        }
        
        void addPostArc(const Arc& arc)
        {
            auto lb = std::lower_bound(post.begin(), post.end(), arc);
            if(lb != post.end() && lb->place == arc.place)
                lb->weight += arc.weight;
            else
                lb = post.insert(lb, arc);
            assert(lb->weight > 0);
            
        }
    };

    struct Place {
        std::vector<uint32_t> consumers; // things consuming
        std::vector<uint32_t> producers; // things producing
        bool skip = false;
        bool inhib = false;
        
        // should be replaced using concepts in c++20
        void addConsumer(uint32_t id)
        {
            auto lb = std::lower_bound(consumers.begin(), consumers.end(), id);
            if(lb == consumers.end() || *lb != id)
                consumers.insert(lb, id);
        }
        
        void addProducer(uint32_t id)
        {
            auto lb = std::lower_bound(producers.begin(), producers.end(), id);
            if(lb == producers.end() || *lb != id)
                producers.insert(lb, id);
        }
    };
}
#endif /* NETSTRUCTURES_H */

