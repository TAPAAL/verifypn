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
 * File:   ptrie.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 18:44
 */

#ifndef PTRIE_H
#define PTRIE_H

#include <stdint.h>
#include <assert.h>
#include <limits>
#include <stack>
#include <string.h>
#include <functional>

#include "binarywrapper.h"
#include "linked_bucket.h"



// direct 2lvl indexing in chunks of ~ 2^32
// takes up ~ 512k (256k on 32bit) for the index.
// when to keep data in bucket or send to heap - just make sure bucket does not overflow

namespace ptrie {
    typedef uint32_t uint;
    typedef unsigned char uchar;

    template<typename D, typename N>
    struct ptrie_el_t {
        N node;
        D data;
        uchar path;
    };

    template<typename N>
    struct ptrie_el_t<void, N> {
        N node;
        uchar path;
    } __attribute__((packed));
    typedef std::pair<bool, size_t> returntype_t;

    struct base_t {
        uchar _path;
        uchar _type;
    } __attribute__((packed));


#define PTRIETPL uint16_t HEAPBOUND, uint16_t SPLITBOUND, size_t ALLOCSIZE, typename T, typename I
    
    template<
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 129,
    size_t ALLOCSIZE = (1024 * 64),
    typename T = void,
    typename I = size_t
    >
    class set {
    protected:

        struct fwdnode_t;
        struct node_t;

        static_assert(HEAPBOUND * (sizeof(fwdnode_t)/sizeof(size_t)) < std::numeric_limits<uint16_t>::max(),
                "HEAPBOUND * (sizeof(fwdnode_t)/sizeof(fwdnode_t)) should be less than 2^16");

        static_assert(SPLITBOUND < sizeof(fwdnode_t),
                "SPLITBOUND should be less than sizeof(fwdnode_t)");

        static_assert(SPLITBOUND > 3,
                "SPLITBOUND MUST BE LARGER THAN 3");

        static_assert(HEAPBOUND > sizeof(size_t),
                "HEAPBOUND MUST BE LARGER THAN sizeof(size_t)");
    protected:

        typedef ptrie_el_t<T, I> entry_t;

        struct bucket_t {

            bucket_t() {
            }
            uchar _data;

            static inline size_t overhead(size_t count, bool hasent) {
                if (hasent)
                    return count * (sizeof (uint16_t) + sizeof (I));
                else
                    return count * (sizeof (uint16_t));
            }

            inline I* entries(uint16_t count, bool hasent) {
                if (hasent) return (I*) (data(0, hasent) + (count * (sizeof (uint16_t))));
                else return NULL;
            }

            inline uchar* data(uint16_t count, bool hasent) {
                return ((uchar*) (&_data)) + overhead(count, hasent);
            }

            inline uint16_t& first(uint16_t count = 0, uint16_t index = 0) {
                return ((uint16_t*) &_data)[index];
            }

        } __attribute__((packed));

        // nodes in the tree

        struct node_t : public base_t {
            uint16_t _totsize = 0;
            uint16_t _count = 0; // bucket-counts
            bucket_t* _data = NULL; // back-pointers to data-array up to date
        } __attribute__((packed));

        struct fwdnode_t : public base_t {
            base_t* _children[256];
            fwdnode_t* _parent;

            base_t*& operator[](size_t i) {
                return _children[i];
            }
        } __attribute__((packed));

        linked_bucket_t<entry_t, ALLOCSIZE>* _entries = NULL;

        fwdnode_t* _root;
    protected:
        node_t* new_node();
        fwdnode_t* new_fwd();

        base_t* fast_forward(const binarywrapper_t& encoding, fwdnode_t** tree_pos, uint& byte);
        bool bucket_search(const binarywrapper_t& encoding, node_t* node, uint& b_index, uint byte);

        bool best_match(const binarywrapper_t& encoding, fwdnode_t** tree_pos, base_t** node, uint& byte, uint& b_index);

        void split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        void split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        static inline uint16_t bytes(const uint16_t d) {
            if (d >= HEAPBOUND) return sizeof (uchar*);
            else return d;
        }

        void init();

        void erase(fwdnode_t* parent, node_t* node, size_t bucketid, int byte);
        bool merge_down(fwdnode_t* parent, node_t* node, int byte);
        void inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> sizes);


    public:
        set();
        ~set();

        returntype_t insert(binarywrapper_t wrapper);
        returntype_t insert(const uchar* data, size_t length);
        returntype_t exists(binarywrapper_t wrapper);
        returntype_t exists(const uchar* data, size_t length);
        bool         erase (binarywrapper_t wrapper);
        bool         erase (const uchar* data, size_t length);


    };

    template<PTRIETPL>
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::~set() {
        std::stack<fwdnode_t*> stack;
        stack.push(_root);
        while(!stack.empty())
        {
            fwdnode_t* next = stack.top();
            stack.pop();
            for(size_t i = 0; i < 256; ++i)
            {
                base_t* child = next->_children[i];
                if(child != next)
                {
                    if(i > 0 && child == next->_children[i-1]) continue;
                    if(child->_type == 255)
                    {
                        stack.push((fwdnode_t*)child);
                    }
                    else
                    {
                        node_t* node = (node_t*) child;
                        // TODO: we should delete data here also!
                        free(node->_data);
                        delete node;
                    }
                }
            }
            delete next;
        }
       delete _entries;
    }

    template<PTRIETPL>
    void set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::init()
    {
        delete _entries;
        if(_entries != NULL)
        {
            _entries = new linked_bucket_t<entry_t, ALLOCSIZE>(1);
        }

        _root = new_fwd();
        _root->_parent = NULL;
        _root->_type = 255;
        _root->_path = 0;

        size_t i = 0;
        for (; i < 256; ++i) (*_root)[i] = _root;
    }

    template<PTRIETPL>
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::set()
    {
        init();
    }

    template<PTRIETPL>
    typename set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::node_t*
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::new_node() {
        return new node_t;
    }

    template<PTRIETPL>
    typename set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::fwdnode_t*
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::new_fwd() {
        return new fwdnode_t;
    }

    template<PTRIETPL>
    base_t*
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::fast_forward(const binarywrapper_t& encoding, fwdnode_t** tree_pos, uint& byte) {
        fwdnode_t* t_pos = *tree_pos;

        uint16_t s = encoding.size(); // TODO remove minus to
        uchar* sc = (uchar*) & s;

        do {
            *tree_pos = t_pos;

            base_t* next;
            if (byte >= 2) next = (*t_pos)[encoding[byte - 2]];
            else next = (*t_pos)[sc[1 - byte]];

            assert(next != NULL);
            if(next == t_pos)
            {
              return t_pos;
            } else
            if (next->_type != 255) {
                return (node_t*) next;
            } else {
                t_pos = static_cast<fwdnode_t*> (next);
                ++byte;
            }
        } while (true);
        assert(false);
    }

    template<PTRIETPL>
    bool set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::bucket_search(const binarywrapper_t& encoding, node_t* node, uint& b_index, uint byte) {
        // run through the stored data in the bucket, looking for matches
        // start by creating an encoding that "points" to the "unmatched"
        // part of the encoding. Notice, this is a shallow copy, no actual
        // heap-allocation happens!
        const bool hasent = _entries != NULL;
        bool found = false;

        uint16_t encsize;
        if (encoding.size() > byte) {
            encsize = byte > 0 ? encoding.size() - byte : encoding.size();
        } else {
            encsize = 0;
        }
        const uchar* raw = encoding.const_raw() + byte;
        uint16_t first;
        uchar* tf = (uchar*) & first;
        if (byte <= 1) {
            first = encoding.size();
            if (byte == 1) {
                first <<= 8;
                tf[0] = encoding[0];
            }
        } else {
            tf[1] = encoding[-2 + byte];
            tf[0] = encoding[-2 + byte + 1];
        }

        bucket_t* bucket = node->_data;
        if (node->_count > 0) {
            size_t offset = 0;
            b_index = 0;
            uint16_t bs = 0;
            uchar* bsc = (uchar*) & bs;
            uint16_t f;
            uchar* fc = (uchar*) & f;
            for (; b_index < node->_count; ++b_index) {
                f = bucket->first(node->_count, b_index);
                if (f >= first) break;
                if (byte > 1) bs = encsize;
                else if (byte == 1) {
                    bs = encoding.size();
                    bsc[0] = fc[1];
                    bs -= 1;

                    //                    bs = bucket->bytes(b_index);
                } else // if byte == 0
                {
                    bsc[0] = fc[0];
                    bsc[1] = fc[1];
                    //                    bs = bucket->bytes(b_index);
                }

                //                assert(bytes(bs) == bucket->bytes(b_index));
                offset += bytes(bs);
            }

            if (b_index >= node->_count ||
                    bucket->first(node->_count, b_index) > first) return false;

            uchar* data = bucket->data(node->_count, hasent);
            for (; b_index < node->_count; ++b_index) {

                size_t b = 0;

                if (bucket->first(node->_count, b_index) > first) break;
                // first is 2 bytes, which is size of counter, the first part of the tree
                // if we reach here, things have same size

                if (encsize < HEAPBOUND) {
                    for (; b < encsize; ++b) {
                        if (data[offset + b] != raw[b]) break;
                    }
                    if (b == encsize) {
                        found = true;
                        break;
                    } else {
                        assert(raw[b] != data[offset + b]);
                        if (raw[b] < data[offset + b]) {
                            found = false;
                            break;
                        }
                        // else continue search
                    }
                } else {
                    uchar* ptr = *((uchar**) (&data[offset]));
                    int cmp = memcmp(ptr, raw, encsize);
                    if (cmp > 0) {
                        found = false;
                        break;
                    }

                    if (cmp == 0) {
                        found = true;
                        break;
                    }
                }
                offset += bytes(encsize);
            }
        } else {
            b_index = 0;
        }
        return found;
    }

    template<PTRIETPL>
    bool set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::best_match(const binarywrapper_t& encoding, fwdnode_t** tree_pos, base_t** node,
            uint& byte, uint& b_index) {
        // run through tree as long as there are branches covering some of 
        // the encoding
        *node = fast_forward(encoding, tree_pos, byte);
        if((size_t)*node != (size_t)*tree_pos) {
            return bucket_search(encoding, (node_t*)*node, b_index, byte);
        } else
        {
            return false;
        }
    }


    template<PTRIETPL>
    void set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte) {

        const uint16_t bucketsize = node->_count;
        if(bucketsize < (sizeof(fwdnode_t) / sizeof(size_t))) return;
        const bool hasent = _entries != NULL;
        node_t lown;
        fwdnode_t* fwd_n = new_fwd();

        //        std::cerr << "SplitFWD " << (locked ? locked : node) << " OP : " << jumppar << " NP : " << fwd_n  << " LOW : " << low_n << std::endl;

        fwd_n->_parent = jumppar;
        fwd_n->_type = 255;
        fwd_n->_path = node->_path;

        lown._path = 0;
        node->_path = binarywrapper_t::_masks[0];
        lown._type = 1;
        node->_type = 1;


        for (size_t i = 0; i < 256; ++i) (*fwd_n)[i] = (locked == NULL ? node : locked);

        (*jumppar)[fwd_n->_path] = fwd_n;

        lown._data = NULL;

        int lcnt = 0;
        int hcnt = 0;
        int lsize = 0;
        int hsize = 0;
        int bcnt = 0;
        bucket_t* bucket = node->_data;
        // get sizes
        uint16_t lengths[sizeof(fwdnode_t)/sizeof(size_t)];
        for (int i = 0; i < bucketsize; ++i) {

            lengths[i] = bsize;
            if (byte < 2) {
                uchar* f = (uchar*)&(bucket->first(bucketsize, i));
                uchar* d = (uchar*)&(lengths[i]);
                if (byte != 0) {
                    lengths[i] += 1;
                    d[0] = f[1];
                    lengths[i] -= 1;
                } else {
                    d[0] = f[0];
                    d[1] = f[1];
                }
            }
            //            assert(lengths[i] == bucket->length(i));

            if ((bucket->first(bucketsize, i) & binarywrapper_t::_masks[0]) == 0) {
                ++lcnt;
                if (lengths[i] < (HEAPBOUND + 1)) {
                    if (lengths[i] > 1) {
                        lsize += lengths[i] - 1;
                    }
                    // otherwise new size = 0
                } else {
                    lsize += bytes(lengths[i]);
                }
            } else {
                ++hcnt;
                if (lengths[i] < (HEAPBOUND + 1)) {
                    hsize += lengths[i] - 1;
                } else {
                    hsize += bytes(lengths[i]);
                }
            }
            bcnt += bytes(lengths[i]);
        }

        //if(bucketsize > 0)free(node->_data);

        // allocate new buckets
        node->_totsize = hsize > 0 ? hsize : 0;
        node->_count = hcnt;
        if (hcnt == 0) node->_data = NULL;
        else node->_data = (bucket_t*) malloc(node->_totsize + 
                bucket_t::overhead(node->_count, hasent));

        lown._totsize = lsize > 0 ? lsize : 0;
        lown._count = lcnt;
        if (lcnt == 0) lown._data = NULL;
        else lown._data = (bucket_t*) malloc(lown._totsize +
                bucket_t::overhead(lown._count, hasent));

        // copy values
        int lbcnt = 0;
        int hbcnt = 0;
        bcnt = 0;
#define LLENGTH(x) lengths[x] - 1
        for (int i = 0; i < bucketsize; ++i) {
            if (i < lown._count) {
                lown._data->first(lown._count, i) = (bucket->first(bucketsize, i) << 8);
                if (lengths[i] > 0) {
                    //                    lown._data->length(i) = LLENGTH(i);
                    uchar* dest = &(lown._data->data(lown._count, hasent)[lbcnt]);
                    if (LLENGTH(i) >= HEAPBOUND) {
                        uchar* data = (uchar*) malloc(LLENGTH(i));
                        memcpy(dest, &data, sizeof (uchar*));
                        dest = data;
                        //                        std::cout << "DATA FOR " << i << " in " << low_n << " IN " << (void*)dest << std::endl;
                    }

                    uchar* src;
                    if (lengths[i] >= HEAPBOUND) {
                        src = *((uchar**)&(bucket->data(bucketsize, hasent)[bcnt]));
                    } else {
                        src = &(bucket->data(bucketsize, hasent)[bcnt]);
                    }

                    uchar* f = (uchar*)&(lown._data->first(lown._count, i));
                    f[0] = src[0];

                    memcpy(dest,
                            &(src[1]),
                            LLENGTH(i));

                    if (lengths[i] >= HEAPBOUND) {
#ifndef NDEBUG
                        if (LLENGTH(i) >= HEAPBOUND) {
                            uchar* tmp = *((uchar**)&(lown._data->data(lown._count, hasent)[lbcnt]));
                            assert(tmp == dest);
                            assert(memcmp(tmp, &(src[1]), LLENGTH(i)) == 0);
                        }
#endif
                        free(src);
                    }

                    lbcnt += bytes(LLENGTH(i));
                }

                //                std::cout << bucket->first(bucketsize, i) << std::endl;
                //                std::cout << i << " NFIRST " << lown._data->first(lown._count, i) << std::endl;

                //                assert(lown._data->length(i) == 0 && lengths[i] <= 1 ||
                //                        lengths[i] - 1 == lown._data->length(i));
            } else {
                int j = i - lown._count;
                node->_data->first(node->_count, j) = (bucket->first(bucketsize, i) << 8);
                //                node->_data->length(j) = lengths[i] - 1;
                if (lengths[i] > 0) {
                    uchar* dest = &(node->_data->data(node->_count, hasent)[hbcnt]);
                    if (LLENGTH(i) >= HEAPBOUND) {
                        uchar* data = (uchar*) malloc(LLENGTH(i));
                        memcpy(dest, &data, sizeof (uchar*));
                        dest = data;
                        //                    std::cout << "DATA FOR " << j << " in " << node << " IN " << (void*)dest << std::endl;
                    }

                    uchar* src;
                    if (lengths[i] < HEAPBOUND) {
                        src = &(bucket->data(bucketsize, hasent)[bcnt]);
                    } else {
                        src = *((uchar**)&(bucket->data(bucketsize, hasent)[bcnt]));
                    }

                    uchar* f = (uchar*) & node->_data->first(node->_count, j);

                    f[0] = src[0];

                    memcpy(dest,
                            &(src[1]),
                            LLENGTH(i));

                    if (lengths[i] >= HEAPBOUND) {
#ifndef NDEBUG
                        if (LLENGTH(i) >= HEAPBOUND) {
                            uchar* tmp = *((uchar**)&(node->_data->data(node->_count, hasent)[hbcnt]));
                            assert(tmp == dest);
                            assert(memcmp(tmp, &(src[1]), LLENGTH(i)) == 0);
                        }
#endif
                        free(src);
                    }
                }

                hbcnt += bytes(LLENGTH(i));
                assert(LLENGTH(i) == lengths[i] - 1);
                //                std::cout << bucket->first(bucketsize, i) << std::endl;
                //                std::cout << i << " NFIRST " << node->_data->first(node->_count, j) << std::endl;

            }

            bcnt += bytes(lengths[i]);

        }

        if (hasent) {
            I* ents = bucket->entries(bucketsize, true);
            for (size_t i = 0; i < bucketsize; ++i) {
                entry_t& ent = _entries->operator[](ents[i]);
                ent.node = (size_t)fwd_n;
                ent.path = (bucket->first(bucketsize, i));
            }
        }

        if (lcnt == 0) {
            if (hasent)
                memcpy(node->_data->entries(bucketsize, true), bucket->entries(bucketsize, true), bucketsize * sizeof (I));
            free(bucket);
            //            std::cout << "SPLIT ALL HIGH" << std::endl;
            lown._data = NULL;
            for (size_t i = 0; i < 128; ++i) (*fwd_n)[i] = fwd_n;
            split_node(node, fwd_n, locked, bsize > 0 ? bsize - 1 : 0, byte + 1);
        }
        else if (hcnt == 0) {
            if (hasent)
                memcpy(lown._data->entries(bucketsize, true), bucket->entries(bucketsize, true), bucketsize * sizeof (I));
            for (size_t i = 128; i < 256; ++i) (*fwd_n)[i] = fwd_n;
            free(bucket);
            //            std::cout << "SPLIT ALL LOW" << std::endl;
            node->_data = lown._data;
            node->_path = lown._path;
            node->_count = lown._count;
            node->_totsize = lown._totsize;
            node->_type = lown._type;
            split_node(node, fwd_n, locked, bsize > 0 ? bsize - 1 : 0, byte + 1);
        } else {
            node_t* low_n = new_node();
            low_n->_data = lown._data;
            low_n->_totsize = lown._totsize;
            low_n->_count = lown._count;
            low_n->_path = lown._path;
            low_n->_type = lown._type;
            for (size_t i = 0; i < 128; ++i) (*fwd_n)[i] = low_n;
            if (hasent) {
                // We are stopping splitting here, so correct entries if needed
                I* ents = bucket->entries(bucketsize, true);

                for (size_t i = 0; i < bucketsize; ++i) {
                    if (i < lown._count) lown._data->entries(lown._count, true)[i] = ents[i];
                    else node->_data->entries(node->_count, true)[i - lown._count] = ents[i];
                }
            }
            free(bucket);
            if(lown._count >= SPLITBOUND && lown._count >= node->_count)
            {
                split_node(low_n, fwd_n, NULL, bsize > 0 ? bsize - 1 : 0, byte + 1);
            }
            if(node->_count >= SPLITBOUND)
            {
                 split_node(node, fwd_n, locked, bsize > 0 ? bsize - 1 : 0, byte + 1);
            }
            //            std::cout << "SPLIT Moving LOW " << lown._count << " TO " << low_n << std::endl;
            //            std::cout << "SPLIT Moving HIGH " << node->_count << " TO " << node << std::endl;

        }
    }

    template<PTRIETPL>
    void set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte) {

        assert(bsize != std::numeric_limits<size_t>::max());
        if (node->_type == 8) // new fwd node!
        {
            //stuff
            split_fwd(node, jumppar, locked, bsize, byte);
            return;
        }

        const uint16_t bucketsize = node->_count;

        node_t hnode;
        //        std::cerr << "Split " << (locked ? locked : node) << " high : " << h_node << std::endl;

        hnode._type = node->_type + 1;
        assert(hnode._type <= 8);
        assert(node->_type <= 8);

        //assert(n_node->_lck == 0);
        assert((node->_path & binarywrapper_t::_masks[node->_type]) == 0);
        hnode._path = node->_path | binarywrapper_t::_masks[node->_type];

        // because we are only really shifting around bits when enc_pos % 8 = 0
        // then we need to find out which bit of the first 8 we are
        // splitting on in the "remainder"
        const uint r_pos = node->_type;
        assert(r_pos < 8);

        // Copy over the data to the new buckets

        int lcnt = 0;
        int hcnt = bucketsize;
        int lsize = 0;

#ifndef NDEBUG
        bool high = false;
#endif

        for (size_t i = 0; i < bucketsize; i++) {

            uchar* f = (uchar*) & node->_data->first(bucketsize, i);
            if ((f[1] & binarywrapper_t::_masks[r_pos]) > 0) {
#ifndef NDEBUG
                high = true;
#else
                break;
#endif
            } else {
                assert(!high);
                ++lcnt;
                --hcnt;
                uint16_t fc;
                if (byte < 2) {
                    uchar* fcc = (uchar*) & fc;
                    if (byte == 0) {
                        fcc[0] = f[0];
                        fcc[1] = f[1];
                    } else {
                        fc = (bsize + byte);
                        fcc[0] = f[1];
                        fc -= byte;
                    }
                } else {
                    fc = bsize;
                }
                //                assert(bytes(fc) == node->_data->bytes(i));
                lsize += bytes(fc);
            }
        }

        bucket_t* old = node->_data;
        // copy over values
        hnode._count = hcnt;
        hnode._totsize = node->_totsize - lsize;
        node->_count = lcnt;
        node->_totsize = lsize;

        if(byte >= 2)
        {
            assert(hnode._totsize == bytes(bsize) * hnode._count);
            assert(node->_totsize == bytes(bsize) * node->_count);
        }

        size_t dist = (hnode._path - node->_path);
        assert(dist > 0);

        node->_type += 1;

        const bool hasent = _entries != NULL;

        if (node->_count == 0) // only high node has data
        {
            //            std::cout << "ALL HIGH" << std::endl;
            for(size_t i = node->_path; i < hnode._path; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = jumppar;
            }

            node->_path = hnode._path;
            node->_count = hnode._count;
            node->_data = old;
            node->_totsize = hnode._totsize;
            node->_type = hnode._type;


            split_node(node, jumppar, locked, bsize, byte);
        }
        else if (hnode._count == 0) // only low node has data
        {
            //            std::cout << "ALL LOW" << std::endl;
            for(size_t i = hnode._path; i < hnode._path + dist; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = jumppar;
            }

            node->_data = old;
            split_node(node, jumppar, locked, bsize, byte);
        } else {
            node_t* h_node = new_node();
            h_node->_count = hnode._count;
            h_node->_type = hnode._type;
            h_node->_path = hnode._path;
            h_node->_totsize = hnode._totsize;
            h_node->_data = hnode._data;

            for(size_t i = hnode._path; i < hnode._path + dist; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = h_node;
            }

            //            std::cout << "Moving LOW " << node->_count << " TO " << node << std::endl;
            //            std::cout << "Moving HIGH " << h_node->_count << " TO " << h_node << std::endl;
            h_node->_data = (bucket_t*) malloc(h_node->_totsize + 
                    bucket_t::overhead(h_node->_count, hasent));
            node->_data = (bucket_t*) malloc(node->_totsize + 
                    bucket_t::overhead(node->_count, hasent));

            // copy firsts
            memcpy(&node->_data->first(node->_count), &(old->first(bucketsize)), sizeof (uint16_t) * node->_count);
            memcpy(&h_node->_data->first(h_node->_count), &(old->first(bucketsize, node->_count)), sizeof (uint16_t) * h_node->_count);

            // copy data
            memcpy(node->_data->data(node->_count, hasent), old->data(bucketsize, hasent), node->_totsize);
            memcpy(h_node->_data->data(h_node->_count, hasent), &(old->data(bucketsize, hasent)[node->_totsize]), h_node->_totsize);

            if (hasent) {
                I* ents = old->entries(bucketsize, true);

                for (size_t i = 0; i < bucketsize; ++i) {
                    if (i < node->_count) node->_data->entries(node->_count, true)[i] = ents[i];
                    else h_node->_data->entries(h_node->_count, true)[i - node->_count] = ents[i];
                }
            }

            free(old);
            if(node->_count >= SPLITBOUND && node->_count >= h_node->_count)
            {
                split_node(node, jumppar, locked, bsize, byte);
            }
            if(h_node->_count >= SPLITBOUND)
            {
                split_node(h_node, jumppar, NULL, bsize, byte);
            }
        }
    }

    template<PTRIETPL>
    std::pair<bool, size_t>
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::exists(const uchar* data, size_t length) {
        assert(length <= 65536);
        binarywrapper_t encoding((uchar*) data, length * 8);
        //        memcpy(encoding.raw()+2, data, length);
        //        length += 2;
        //        memcpy(encoding.raw(), &length, 2);

        uint b_index = 0;

        fwdnode_t* fwd = _root;
        base_t* base = NULL;
        uint byte = 0;

        b_index = 0;
        bool res = best_match(encoding, &fwd, &base, byte, b_index);
        returntype_t ret = returntype_t(res, std::numeric_limits<size_t>::max());
        if((size_t)fwd != (size_t)base) {
            node_t* node = (node_t*)base;
            if (_entries != NULL && res) {
                ret.second = node->_data->entries(node->_count, true)[b_index];
            }
        }
        return ret;
    }

    template<PTRIETPL>
    returntype_t
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::insert(const uchar* data, size_t length) {
        assert(length <= 65536);
        binarywrapper_t e2((uchar*) data, length * 8);
        const bool hasent = _entries != NULL;
        //        binarywrapper_t encoding(length*8+16);
        //        memcpy(encoding.raw()+2, data, length);
        //        length += 2;
        //        memcpy(encoding.raw(), &length, 2);

        uint b_index = 0;

        fwdnode_t* fwd = _root;
        node_t* node = NULL;
        base_t* base = NULL;
        uint byte = 0;

        bool res = best_match(e2, &fwd, &base, byte, b_index);
        if (res) { // We are not inserting duplicates, semantics of PTrie is a set.
            returntype_t ret(false, 0);
            if (hasent) {
                node = (node_t*)base;
                ret = returntype_t(false, node->_data->entries(node->_count, true)[b_index]);
            }
            return ret;
        }

        if((size_t)base == (size_t)fwd)
        {
            node = new_node();
            node->_count = 0;
            node->_data = NULL;
            node->_type = 0;
            node->_path = 0;
            assert(node);

            size_t s = e2.size();
            uchar* sc = (uchar*) & s;
            uchar b = (byte < 2 ? sc[1 - byte] : e2[byte-2]);

            uchar min = b;
            uchar max = b;
            int bit = 8;
            bool stop = false;
            do {
                --bit;

                min = min & (~binarywrapper_t::_masks[bit]);
                max |= binarywrapper_t::_masks[bit];
                for (int i = min; i <= max ; ++i) {
                    if(fwd->_children[i] != fwd)
                    {
                        max = (max & ~binarywrapper_t::_masks[bit]) | (binarywrapper_t::_masks[bit] & b);
                        min = min | (binarywrapper_t::_masks[bit] & b);
                        bit += 1;
                        stop = true;
                        break;
                    }
                }
            } while(bit > 0 && !stop);

            for (size_t i = min; i <= max; ++i) (*fwd)[i] = node;
            node->_path = min;
            node->_type = bit;
        } else
        {
            node = (node_t*)base;
        }


        // shallow copy
        binarywrapper_t nenc(e2.const_raw(),
                (e2.size() - byte)*8,
                byte * 8,
                e2.size() * 8);

//                std::cout << "COULD NOT FIND of size " << e2.size() << std::endl;
//                e2.print(std::cout);
//                std::cout << "Inserted into " << node << std::endl;

        // make a new bucket, add new entry, copy over old data

        uint nbucketcount = node->_count + 1;
        uint nitemsize = nenc.size();
        bool copyval = true;
        if (nitemsize >= HEAPBOUND) {
            copyval = false;
            nitemsize = sizeof (uchar*);
        }

        uint nbucketsize = node->_totsize + nitemsize;

        bucket_t* nbucket = (bucket_t*) malloc(nbucketsize + 
                bucket_t::overhead(nbucketcount, hasent));

        // copy over old "firsts"
        memcpy(&nbucket->first(nbucketcount), &(node->_data->first(node->_count)), b_index * sizeof (uint16_t));
        memcpy(&(nbucket->first(nbucketcount, b_index + 1)), &(node->_data->first(node->_count, b_index)),
                (node->_count - b_index) * sizeof (uint16_t));

        uchar* f = (uchar*) & nbucket->first(nbucketcount, b_index);
        if (byte >= 2) {
            f[1] = e2[-2 + byte];
            f[0] = e2[-2 + byte + 1];
        } else {
            nbucket->first(nbucketcount, b_index) = e2.size();
            if (byte == 1) {
                nbucket->first(nbucketcount, b_index) <<= 8;
                f[0] = e2[0];
            }
        }

        size_t entry = 0;
        if (hasent) {
            // copy over entries
            memcpy(nbucket->entries(nbucketcount, true), node->_data->entries(node->_count, true), b_index * sizeof (I));
            memcpy(&(nbucket->entries(nbucketcount, true)[b_index + 1]), &(node->_data->entries(node->_count, true)[b_index]),
                    (node->_count - b_index) * sizeof (I));

            entry = nbucket->entries(nbucketcount, true)[b_index] = _entries->next(0);
            entry_t& ent = _entries->operator[](entry);
            ent.node = (size_t)fwd;
            ent.path = (nbucket->first(nbucketcount, b_index) >> 8);
        }



        uint tmpsize = 0;
        if (byte >= 2) tmpsize = b_index * bytes(nenc.size());
        else {
            uint16_t o = e2.size();
            for (size_t i = 0; i < b_index; ++i) {

                uint16_t f = node->_data->first(nbucketcount - 1, i);
                uchar* fc = (uchar*) & f;
                uchar* oc = (uchar*) & o;
                if (byte != 0) {
                    f >>= 8;
                    fc[1] = oc[1];
                    f -= 1;
                }
                tmpsize += bytes(f);
                //                assert(bytes(f) == nbucket->bytes(i));
            }
        }
        // copy over old data
        memcpy(nbucket->data(nbucketcount, hasent),
                node->_data->data(node->_count, hasent), tmpsize);

        memcpy(&(nbucket->data(nbucketcount, hasent)[tmpsize + nitemsize]),
                &(node->_data->data(node->_count, hasent)[tmpsize]), (node->_totsize - tmpsize));

        // copy over new data
        if (copyval) {
            memcpy(&(nbucket->data(nbucketcount, hasent)[tmpsize]),
                    nenc.const_raw(), nenc.size());
        } else {
            // alloc space
            uchar* data = (uchar*) malloc(nenc.size());

            // copy data to heap
            memcpy(data, nenc.const_raw(), nenc.size());

            // copy pointer in
            memcpy(&(nbucket->data(nbucketcount, hasent)[tmpsize]),
                    &data, sizeof (uchar*));
        }

        // if needed, split the node 

        free(node->_data);
        node->_data = nbucket;
        node->_count = nbucketcount;
        node->_totsize = nbucketsize;

        //        std::cout << " FIRST " << tmp._data->first(nbucketcount, b_index) << std::endl;
        //        std::cout << "NODE " << node << " SIZE : " << nbucketsize << std::endl;
        if (node->_count >= SPLITBOUND) {
            // copy over data to we can work while readers finish
            // we have to wait for readers to finish for 
            // tree extension
            split_node(node, fwd, node, nenc.size(), byte);
        }

#ifndef NDEBUG        
        for (int i = byte - 1; i >= 2; --i) {
            assert(fwd != NULL);
            assert(e2[-2 + i] == fwd->_path);
            assert(fwd->_parent == NULL || fwd->_parent->_children[fwd->_path] == fwd);
            fwd = fwd->_parent;

        }
        auto r = exists(data, length);
        if (!r.first) {

            r = exists(data, length);
            assert(false);
        }
#endif
        return returntype_t(true, entry);
    }

    template<PTRIETPL>
    returntype_t
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::insert(binarywrapper_t wrapper) {
        return insert(wrapper.raw(), wrapper.size());
    }

    template<PTRIETPL>
    returntype_t
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::exists(binarywrapper_t wrapper) {
        return exists(wrapper.raw(), wrapper.size());
    }

    
        template<PTRIETPL>
    void
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> _sizes)
    {
        const bool hasent = _entries != NULL;
        bucket_t *nbucket = node->_data;
        if(totsize > 0) {
            nbucket = (bucket_t *) malloc(totsize +
                                          bucket_t::overhead(node->_count,
                                                             hasent));
        }

        size_t dcnt = 0;
        size_t ocnt = 0;
        for(size_t i = 0; i < node->_count; ++i)
        {
            auto const size = _sizes(i);
            uchar* f = (uchar*)&nbucket->first(node->_count, i);
            nbucket->first(node->_count, i) = node->_data->first(node->_count, i);
            uchar push = f[0];
            f[0] = f[1];
            f[1] = topush;
            // in some cases we need to expand to heap here!
            if(size > 0)
            {
                if(size < HEAPBOUND)
                {
                    nbucket->data(node->_count, hasent)[dcnt] = push;
                    dcnt += 1;
                }
                if(size < HEAPBOUND && size > 1)
                {
                    memcpy(&(nbucket->data(node->_count, hasent)[dcnt]),
                               &(node->_data->data(node->_count, hasent)[ocnt]),
                                       size - 1);
                    ocnt += size - 1;
                    dcnt += size - 1;
                }
                else if(size >= HEAPBOUND)
                {
                    uchar* src = NULL;
                    uchar* dest = (uchar*)malloc(size);
                    memcpy(&(nbucket->data(node->_count, hasent)[dcnt]), &dest, sizeof(size_t));
                    ++dest;
                    dcnt += sizeof(size_t);
                    if(size == HEAPBOUND)
                    {
                        src = &(node->_data->data(node->_count, hasent)[ocnt]);
                        memcpy(dest, src, size - 1);
                        ocnt += size - 1;
                    }
                    else
                    {
                        assert(size > HEAPBOUND);
                        // allready on heap, but we need to expand it
                        src = *(uchar**)&(node->_data->data(node->_count, hasent)[ocnt]);
                        memcpy(dest, src, size - 1);
                        ocnt += sizeof(size_t);
                    }
                    --dest;
                    dest[0] = push;
                }
            }
            // also, handle entries here!
        }

        assert(ocnt == node->_totsize);
        assert(totsize == dcnt);

        if(nbucket != node->_data) free(node->_data);

        node->_data = nbucket;
    }


    template<PTRIETPL>
    bool
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::merge_down(fwdnode_t* parent, node_t* node, int on_heap)
    {
        const bool hasent = _entries != NULL;
        if(node->_type == 0)
        {
            if(node->_count < SPLITBOUND/3) return true;
            if(node->_count == 0)
            {
                for(size_t i = 0; i < 256; ++i) parent->_children[i] = parent;
                delete node;
                do {
                    if (parent != this->_root) {
                        // we can remove fwd and go back one level
                        parent->_parent->_children[parent->_path] = parent->_parent;
                        ++on_heap;
                        fwdnode_t* next = parent->_parent;
                        delete parent;
                        parent = next;
                        base_t* other = parent;
                        for(size_t i = 0; i < 256; ++i)
                        {
                            if(parent->_children[i] != parent && other != parent->_children[i])
                            {
                                if(other != parent)
                                {
                                    other = NULL;
                                    break;
                                }
                                else
                                {
                                    other = parent->_children[i];
                                }
                            }
                        }

                        if(other == NULL)
                        {
                            return true;
                        }
                        else if(other->_type != 255)
                        {
                            node = (node_t*)other;
                            return merge_down(parent, node, on_heap);
                        }
                        else if(other != parent)
                        {
                            assert(other->_type == 255);
                            return true;
                        }

                    } else {
                        return true;
                    }
                } while(true);
            }
            else if(parent != this->_root)
            {
                // we need to re-add path to items here.
                if(parent->_parent == this->_root) {
                    // something
                    uint16_t sizes[256];
                    size_t totsize = 0;
                    for(size_t i = 0; i < node->_count; ++i)
                    {
                        uint16_t t = 0;
                        uchar* tc = (uchar*)&t;
                        uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                        tc[0] = fc[1];
                        tc[1] = parent->_path;
                        sizes[i] = t;
                        totsize += bytes(sizes[i]);
                    }

                    inject_byte(node, parent->_path, totsize, [&sizes](size_t i )
                    {
                        return sizes[i];
                    });

                    node->_path = parent->_path;
                    parent->_parent->_children[node->_path] = node;
                    node->_type = 8;
                    node->_totsize = totsize;
                    fwdnode_t* next = parent->_parent;
                    delete parent;
                    return merge_down(next, node, on_heap + 1);
                }
                else
                {
                    assert(node->_count > 0);
                    if(on_heap == std::numeric_limits<int>::min())
                    {
                        int depth = 0;
                        uint16_t length = 0;
                        uchar* l = (uchar*)&length;
                        fwdnode_t* tmp = parent;

                        while(tmp != this->_root)
                        {
                            l[0] = l[1];
                            l[1] = tmp->_path;
                            tmp = tmp->_parent;
                            ++depth;
                        }
                        assert(length + 1 >= depth);
                        on_heap = length;
                        on_heap -= depth;
                    }

                    on_heap += 1;
                    // first copy in path to firsts.

                    assert(on_heap >= 0);
                    node->_path = parent->_path;
                    parent->_parent->_children[node->_path] = node;
                    fwdnode_t* next = parent->_parent;
                    delete parent;
                    parent = next;
                    node->_type = 8;

                    if(on_heap == 0)
                    {
                        for(size_t i = 0; i < node->_count; ++i)
                        {
                            uchar* f = (uchar*)&node->_data->first(node->_count, i);
                            f[0] = f[1];
                            f[1] = node->_path;
                        }
                    }
                    else if(on_heap > 0)
                    {
                        size_t nbucketsize = 0;
                        if(on_heap >= HEAPBOUND)
                        {
                            nbucketsize = node->_count * sizeof(size_t);
                        }
                        else//if(on_heap < HEAPBOUND)
                        {
                            assert(on_heap >= 0);
                            nbucketsize = on_heap * node->_count;
                        }

                        inject_byte(node, node->_path, nbucketsize, [on_heap](size_t)
                        {
                            return on_heap;
                        });

                        node->_totsize = nbucketsize;
                    }
                    return merge_down(next, node, on_heap + 1);
                }
            }
            if(parent != this->_root)
            {
                assert(node->_count > 0);
                return merge_down(parent->_parent, node, on_heap);
            }
        }
        else
        {
            if(node->_count > SPLITBOUND / 3) return true;
            uchar path = node->_path;
            base_t* child;
            if(path & binarywrapper_t::_masks[node->_type - 1])
            {
                child = parent->_children[
                         path & ~binarywrapper_t::_masks[node->_type - 1]];
            }
            else
            {
                child = parent->_children[
                        path | binarywrapper_t::_masks[node->_type - 1]];
            }

            assert(node != child);

            if(child->_type != node->_type && child->_type != 255)
            {
                // The other node is not ready for merging yet.
                assert(child->_type > node->_type);
                return false;
            }
            else
            {

                if(child->_type != 255) {
                    node_t *other = (node_t *) child;

                    const uint nbucketcount = node->_count + other->_count;
                    const uint nbucketsize = node->_totsize + other->_totsize;

                    if (nbucketcount >= SPLITBOUND)
                        return false;

                    bucket_t *nbucket = (bucket_t *) malloc(nbucketsize +
                                                            bucket_t::overhead(nbucketcount, hasent));
                    node_t *first = node;
                    node_t *second = other;
                    if (path & binarywrapper_t::_masks[node->_type - 1]) {
                        std::swap(first, second);
                    }

                    memcpy(&nbucket->first(nbucketcount),
                           &(first->_data->first(first->_count)),
                           first->_count * sizeof(uint16_t));

                    memcpy(&(nbucket->first(nbucketcount, first->_count)),
                           &(second->_data->first(second->_count)),
                           second->_count * sizeof(uint16_t));

                    if (hasent) {
                        // copy over entries
                        memcpy(nbucket->entries(nbucketcount, true),
                               first->_data->entries(first->_count, true),
                               first->_count * sizeof(I));
                        memcpy(&(nbucket->entries(nbucketcount, true)[first->_count]),
                               second->_data->entries(second->_count, true),
                               second->_count * sizeof(I));

                    }

                    // copy over old data
                    if (nbucketsize > 0) {
                        memcpy(nbucket->data(nbucketcount, hasent),
                               first->_data->data(first->_count, hasent), first->_totsize);

                        memcpy(&(nbucket->data(nbucketcount, hasent)[first->_totsize]),
                               second->_data->data(second->_count, hasent), second->_totsize);

                    }
                    free(node->_data);
                    node->_data = nbucket;
                    node->_totsize = nbucketsize;
                    node->_count = nbucketcount;
                }
                uchar from = node->_path & ~binarywrapper_t::_masks[node->_type - 1];
                uchar to = from;
                for(size_t i = node->_type - 1; i < 8; ++i) {
                    to = to | binarywrapper_t::_masks[i];
                }

                if(child->_type == 255)
                {
                    if(child != parent) return true;
                    for(size_t i = from; i <= to; ++i)
                    {
                        if( parent->_children[i] != child &&
                            parent->_children[i] != node)
                            return  true;
                    }
                }

                node->_type -= 1;
                node->_path = from;

                for(size_t i = from; i <= to; ++i)
                {
                    assert(parent->_children[i] == child ||
                       parent->_children[i] == node);
                    parent->_children[i] = node;
                }
                return merge_down(parent, node, on_heap);
            }
        }
        return true;
    }

    template<PTRIETPL>
    void
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::erase(fwdnode_t* parent, node_t* node, size_t bindex, int on_heap)
    {
        const bool hasent = _entries != NULL;

        // first find size and amount before
        uint16_t size = 0;
        uint16_t before = 0;
        if (parent == this->_root)
        {
            for(size_t i = 0; i < bindex; ++i)
            {
               before += bytes(node->_data->first(node->_count, i));
            }
            size = node->_data->first(node->_count, bindex);
        }
        else if(parent->_parent == this->_root) {
             for(size_t i = 0; i <= bindex; ++i)
             {
                 uint16_t t = 0;
                 uchar* tc = (uchar*)&t;
                 uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                 tc[0] = fc[1];
                 tc[1] = parent->_path;
                 --t;
                 if(i == bindex) size = t;
                 else before += bytes(t);
             }
        } else {
            assert(on_heap != std::numeric_limits<int>::min());
            size = on_heap > 0 ? on_heap : 0;
            before = size * bindex;
        }

        // got sizes, now we can remove data if we point to anything
        if(size >= HEAPBOUND)
        {
            before = sizeof(size_t)*bindex;
            uchar* src = *((uchar**)&(node->_data->data(node->_count, hasent)[before]));
            free(src);
            size = sizeof(size_t);
        }

        uint nbucketcount = node->_count - 1;
        if(nbucketcount > 0) {
            uint nbucketsize = node->_totsize - size;

            bucket_t *nbucket = (bucket_t *) malloc(nbucketsize +
                                                    bucket_t::overhead(nbucketcount, hasent));

            // copy over old "firsts", [0,bindex) to [0,bindex) then (bindex,node->_count) to [bindex, nbucketcount)
            memcpy(&nbucket->first(nbucketcount),
                   &(node->_data->first(node->_count)),
                   bindex * sizeof(uint16_t));

            memcpy(&(nbucket->first(nbucketcount, bindex)),
                       &(node->_data->first(node->_count, bindex + 1)),
                       (nbucketcount - bindex) * sizeof(uint16_t));

            if (hasent) {
                // copy over entries
                memcpy(nbucket->entries(nbucketcount, true),
                       node->_data->entries(node->_count, true),
                       bindex * sizeof(I));
                memcpy(&(nbucket->entries(nbucketcount, true)[bindex]),
                       &(node->_data->entries(node->_count, true)[bindex + 1]),
                       (nbucketcount - bindex) * sizeof(I));

                // copy back entries here in _entries!
                // TODO fixme!
            }

            // copy over old data
            if (nbucketsize > 0) {
                memcpy(nbucket->data(nbucketcount, hasent),
                       node->_data->data(node->_count, hasent), before);
                assert(nbucketsize >= before);
                memcpy(&(nbucket->data(nbucketcount, hasent)[before]),
                       &(node->_data->data(node->_count, hasent)[before + size]),
                       (nbucketsize - before));

            }
            free(node->_data);
            node->_data = nbucket;
            node->_count = nbucketcount;
            node->_totsize -= size;

        }
        else
        {
            free(node->_data);
            node->_data = NULL;
            node->_count = 0;
            node->_totsize = 0;
        }

        merge_down(parent, node, on_heap);
    }

    template<PTRIETPL>
    bool
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::erase(binarywrapper_t encoding)
    {
        assert(encoding.size() <= 65536);
        uint b_index = 0;

        fwdnode_t* fwd = this->_root;
        base_t* base = NULL;
        uint byte = 0;

        b_index = 0;
        bool res = this->best_match(encoding, &fwd, &base, byte, b_index);
        if(!res || (size_t)fwd == (size_t)base)
        {
            assert(!this->exists(encoding).first);
            return false;
        }
        else
        {
            int onheap = encoding.size();
            onheap -= byte;
            erase(fwd, (node_t *) base, b_index, onheap);
            assert(!this->exists(encoding).first);
            return true;
        }
    }

    template<PTRIETPL>
    bool
    set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I>::erase(const uchar *data, size_t length)
    {
        binarywrapper_t b((uchar*)data, length*8);
        return erase(b);
    }


}



#endif /* PTRIE_H */
