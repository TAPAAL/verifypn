/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
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

/* 
 * File:   ptrie_map.h
 * Author: Peter G. Jensen
 *
 *
 * Created on 06 October 2016, 13:51
 */

#ifndef PTRIE_MAP_H
#define PTRIE_MAP_H
#include "ptrie.h"


namespace ptrie {

    template<
    uint16_t HEAPBOUND = 128,
    uint16_t SPLITBOUND = 128,
    size_t ALLOCSIZE = (1024 * 64),
    size_t FWDALLOC = 256,
    typename T = void,
    typename I = size_t
    >
    class set_stable : public set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, FWDALLOC, T, I> {
        using pt = set<PTRIEDEF>;
    public:
        set_stable() : pt()
        {
            this->_entries = new linked_bucket_t<typename pt::entry_t, ALLOCSIZE>(1);
        }

        size_t size() const {
            return this->_entries->size();
        }

        size_t unpack(I index, uchar* destination);
    };

    template<PTRIETPL>
    size_t
    set_stable<PTRIEDEF>::unpack(I index, uchar* destination) {
        typename pt::node_t* node = NULL;
        typename pt::fwdnode_t* par = NULL;
        // we can find size without bothering anyone (to much)        
        std::stack<uchar> path;
        size_t bindex = 0;
        {
#ifndef NDEBUG
            bool found = false;
#endif
            typename pt::entry_t& ent = this->_entries->operator[](index);
            if(sizeof(I) == sizeof(size_t)) par = (typename pt::fwdnode_t*)ent.node;
            else par = &this->_fwd->operator [](ent.node);
            node = (typename pt::node_t*)par->_children[ent.path];
            typename pt::bucket_t* bckt = node->_data;
            I* ents = bckt->entries(node->_count, true);
            for (size_t i = 0; i < node->_count; ++i) {
                if (ents[i] == index) {
                    bindex = i;
#ifndef NDEBUG
                    found = true;
#endif
                    break;
                }
            }
            assert(found);
        }



        while (par != this->_root) {
            path.push(par->_path);
            par = par->_parent;
        }

        uint16_t size = 0;
        size_t offset = 0;
        size_t ps = path.size();
        if (ps <= 1) {
            size = node->_data->first(0, bindex);
            if (ps == 1) {
                size >>= 8;
                uchar* bs = (uchar*) & size;
                bs[1] = path.top();
                path.pop();
            }

            uint16_t o = size;
            for (size_t i = 0; i < bindex; ++i) {

                uint16_t f = node->_data->first(0, i);
                uchar* fc = (uchar*) & f;
                uchar* oc = (uchar*) & o;
                if (ps != 0) {
                    f >>= 8;
                    fc[1] = oc[1];
                    f -= 1;
                }
                offset += pt::bytes(f);
                //                assert(bytes(f) == nbucket->bytes(i));
            }
        } else {
            uchar* bs = (uchar*) & size;
            bs[1] = path.top();
            path.pop();
            bs[0] = path.top();
            path.pop();
            offset = (pt::bytes(size - ps) * bindex);
        }


        if (size > ps) {
            uchar* src;
            if ((size - ps) >= HEAPBOUND) {
                src = *((uchar**)&(node->_data->data(node->_count, true)[offset]));
            } else {
                src = &(node->_data->data(node->_count, true)[offset]);
            }

            memcpy(&(destination[ps]), src, (size - ps));
        }

        uint16_t first = node->_data->first(0, bindex);

        size_t pos = 0;
        while (!path.empty()) {
            destination[pos] = path.top();
            path.pop();
            ++pos;
        }


        if (ps > 0) {
            uchar* fc = (uchar*) & first;
            if (ps > 1) {
                destination[pos] = fc[1];
                ++pos;
            }
            destination[pos] = fc[0];
            ++pos;
        }
        
        return size;
    }
}


#endif /* PTRIE_MAP_H */