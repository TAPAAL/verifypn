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


#include <assert.h>
#include <limits>
#include <stack>
#include <string.h>

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


#define PTRIETPL uint16_t HEAPBOUND, uint16_t SPLITBOUND, size_t ALLOCSIZE, size_t FWDALLOC, typename T, typename I
#define PTRIEDEF HEAPBOUND, SPLITBOUND, ALLOCSIZE, FWDALLOC, T, I

    template<
    uint16_t HEAPBOUND = 128,
    uint16_t SPLITBOUND = 128,
    size_t ALLOCSIZE = (1024 * 64),
    size_t FWDALLOC = 256,
    typename T = void,
    typename I = size_t
    >
    class set {
        static_assert(HEAPBOUND * SPLITBOUND < std::numeric_limits<uint16_t>::max(),
                "HEAPBOUND * SPLITBOUND should be less than 2^16");
    protected:
        static constexpr const uint16_t RLOCK = 1;
        static constexpr const uint16_t WLOCK = 256;

        struct node_t;
        struct fwdnode_t;

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

        struct base_t {
            uchar _path;
            uchar _type;
        } __attribute__((packed));

        // nodes in the tree

        struct node_t : public base_t {
            uint16_t _totsize = 0;
            uint16_t _count = 0; // bucket-counts
            bucket_t* _data = NULL; // back-pointers to data-array up to date
        } __attribute__((packed));

        struct fwdnode_t : public base_t {
            I _index;
            base_t* _children[256];
            fwdnode_t* _parent;

            base_t*& operator[](size_t i) {
                return _children[i];
            }
        } __attribute__((packed));

        linked_bucket_t<entry_t, ALLOCSIZE>* _entries = NULL;
        linked_bucket_t<node_t, ALLOCSIZE>* _nodes = NULL; // small, so alloc a lot
        linked_bucket_t<fwdnode_t, FWDALLOC>* _fwd = NULL; // big, so assume we do not need a lot of these!

        fwdnode_t* _root;
    protected:
        node_t* new_node(size_t thread);

        fwdnode_t* new_fwd(size_t thread);

        node_t* fast_forward(const binarywrapper_t& encoding, fwdnode_t** tree_pos, uint& byte);
        bool bucket_search(const binarywrapper_t& encoding, node_t* node, uint& b_index, uint byte);

        bool best_match(const binarywrapper_t& encoding, fwdnode_t** tree_pos, node_t** node, uint& byte, uint& b_index);

        void split_node(node_t* node, size_t thread, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        void split_fwd(node_t* node, size_t thread, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        static inline uint16_t bytes(const uint16_t d) {
            if (d >= HEAPBOUND) return sizeof (uchar*);
            else return d;
        }

    public:
        set();
        ~set();

        returntype_t insert(binarywrapper_t wrapper, size_t thread = 0);
        returntype_t insert(const uchar* data, uint16_t length, size_t thread = 0);
        returntype_t exists(binarywrapper_t wrapper, size_t thread = 0);
        returntype_t exists(const uchar* data, uint16_t length);
    };

    template<PTRIETPL>
    set<PTRIEDEF>::~set() {

        size_t n = _nodes->size();
        for (size_t i = 0; i < n; ++i) {
            node_t& n = _nodes->operator[](i);
            for (size_t i = 0; i < n._count; ++i) {
                // TODO: Cleanup properly here - if things are on the heap!
                /*if(n._data->length(i) >= HEAPBOUND)
                {
                    uchar* ptr = *((uchar**)(n._data->data(n._count, i)));
                    free(ptr);
                }*/
            }
            free(n._data);
        }
        delete _nodes;
        delete _fwd;
        delete _entries;
    }

    template<PTRIETPL>
    set<PTRIEDEF>::set()
    {
        _nodes = new linked_bucket_t<node_t, ALLOCSIZE>(1);
        _fwd = new linked_bucket_t<fwdnode_t, FWDALLOC>(1);

        _root = new_fwd(0);
        _root->_parent = NULL;
        _root->_type = 0;
        _root->_path = 0;

        node_t* low = new_node(0);
        node_t* high = new_node(0);

        low->_count = high->_count = 0;
        low->_data = high->_data = NULL;
        low->_type = high->_type = 1;
        low->_path = 0;
        high->_path = binarywrapper_t::_masks[0];

        size_t i = 0;
        for (; i < 128; ++i) (*_root)[i] = low;
        for (; i < 256; ++i) (*_root)[i] = high;
    }

    template<PTRIETPL>
    typename set<PTRIEDEF>::node_t*
    set<PTRIEDEF>::new_node(size_t thread) {
        size_t n = _nodes->next(thread);
        node_t* no = &_nodes->operator[](n);
        //        std::cout << "NEW NODE >> " << no << std::endl;
        return no;
    }

    template<PTRIETPL>
    typename set<PTRIEDEF>::fwdnode_t*
    set<PTRIEDEF>::new_fwd(size_t thread) {
        size_t n = _fwd->next(thread);
        fwdnode_t* node = &_fwd->operator[](n);
        node->_index = n;
        return node;
    }

    template<PTRIETPL>
    typename set<PTRIEDEF>::node_t*
    set<PTRIEDEF>::fast_forward(const binarywrapper_t& encoding, fwdnode_t** tree_pos, uint& byte) {
        fwdnode_t* t_pos = *tree_pos;

        uint16_t s = encoding.size(); // TODO remove minus to
        uchar* sc = (uchar*) & s;

        do {
            *tree_pos = t_pos;

            base_t* next;
            if (byte >= 2) next = (*t_pos)[encoding[byte - 2]];
            else next = (*t_pos)[sc[1 - byte]];

            assert(next != NULL);
            if (next->_type != 0) {
                return (node_t*) next;
            } else {
                t_pos = static_cast<fwdnode_t*> (next);
                ++byte;
            }
        } while (true);
        assert(false);
    }

    template<PTRIETPL>
    bool set<PTRIEDEF>::bucket_search(const binarywrapper_t& encoding, node_t* node, uint& b_index, uint byte) {
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
    bool set<PTRIEDEF>::best_match(const binarywrapper_t& encoding, fwdnode_t** tree_pos, node_t** node,
            uint& byte, uint& b_index) {
        // run through tree as long as there are branches covering some of 
        // the encoding
        *node = fast_forward(encoding, tree_pos, byte);
        return bucket_search(encoding, *node, b_index, byte);
    }


    template<PTRIETPL>
    void set<PTRIEDEF>::split_fwd(node_t* node, size_t thread, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte) {

        const bool hasent = _entries != NULL;
        node_t* low_n = new_node(thread);
        fwdnode_t* fwd_n = new_fwd(thread);

        //        std::cerr << "SplitFWD " << (locked ? locked : node) << " OP : " << jumppar << " NP : " << fwd_n  << " LOW : " << low_n << std::endl;

        fwd_n->_parent = jumppar;
        fwd_n->_type = 0;
        fwd_n->_path = node->_path;

        low_n->_path = 0;
        node->_path = binarywrapper_t::_masks[0];
        low_n->_type = 1;
        node->_type = 1;

        size_t i = 0;
        for (; i < 128; ++i) (*fwd_n)[i] = low_n;
        for (; i < 256; ++i) (*fwd_n)[i] = (locked == NULL ? node : locked);

        (*jumppar)[fwd_n->_path] = fwd_n;

        low_n->_data = NULL;

        int lcnt = 0;
        int hcnt = 0;
        int lsize = 0;
        int hsize = 0;
        int bcnt = 0;
        bucket_t* bucket = node->_data;
        // get sizes
        uint16_t lengths[SPLITBOUND];
        for (int i = 0; i < SPLITBOUND; ++i) {

            lengths[i] = bsize;
            if (byte < 2) {
                uchar* f = (uchar*)&(bucket->first(SPLITBOUND, i));
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

            if ((bucket->first(SPLITBOUND, i) & binarywrapper_t::_masks[0]) == 0) {
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
        node->_totsize = hsize;
        node->_count = hcnt;
        if (hcnt == 0) node->_data = NULL;
        else node->_data = (bucket_t*) malloc(node->_totsize + 
                bucket_t::overhead(node->_count, hasent));

        low_n->_totsize = lsize;
        low_n->_count = lcnt;
        if (lcnt == 0) low_n->_data = NULL;
        else low_n->_data = (bucket_t*) malloc(low_n->_totsize +
                bucket_t::overhead(low_n->_count, hasent));

        // copy values
        int lbcnt = 0;
        int hbcnt = 0;
        bcnt = 0;
#define LLENGTH(x) lengths[x] - 1
        for (int i = 0; i < SPLITBOUND; ++i) {
            if (i < low_n->_count) {
                low_n->_data->first(low_n->_count, i) = (bucket->first(SPLITBOUND, i) << 8);
                if (lengths[i] > 0) {
                    //                    low_n->_data->length(i) = LLENGTH(i);
                    uchar* dest = &(low_n->_data->data(low_n->_count, hasent)[lbcnt]);
                    if (LLENGTH(i) >= HEAPBOUND) {
                        uchar* data = (uchar*) malloc(LLENGTH(i));
                        memcpy(dest, &data, sizeof (uchar*));
                        dest = data;
                        //                        std::cout << "DATA FOR " << i << " in " << low_n << " IN " << (void*)dest << std::endl;
                    }

                    uchar* src;
                    if (lengths[i] >= HEAPBOUND) {
                        src = *((uchar**)&(bucket->data(SPLITBOUND, hasent)[bcnt]));
                    } else {
                        src = &(bucket->data(SPLITBOUND, hasent)[bcnt]);
                    }

                    uchar* f = (uchar*)&(low_n->_data->first(low_n->_count, i));
                    f[0] = src[0];

                    memcpy(dest,
                            &(src[1]),
                            LLENGTH(i));

                    if (lengths[i] >= HEAPBOUND) {
#ifndef NDEBUG
                        if (LLENGTH(i) >= HEAPBOUND) {
                            uchar* tmp = *((uchar**)&(low_n->_data->data(low_n->_count, hasent)[lbcnt]));
                            assert(tmp == dest);
                            assert(memcmp(tmp, &(src[1]), LLENGTH(i)) == 0);
                        }
#endif
                        free(src);
                    }

                    lbcnt += bytes(LLENGTH(i));
                }

                //                std::cout << bucket->first(SPLITBOUND, i) << std::endl;
                //                std::cout << i << " NFIRST " << low_n->_data->first(low_n->_count, i) << std::endl;

                //                assert(low_n->_data->length(i) == 0 && lengths[i] <= 1 ||
                //                        lengths[i] - 1 == low_n->_data->length(i));
            } else {
                int j = i - low_n->_count;
                node->_data->first(node->_count, j) = (bucket->first(SPLITBOUND, i) << 8);
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
                        src = &(bucket->data(SPLITBOUND, hasent)[bcnt]);
                    } else {
                        src = *((uchar**)&(bucket->data(SPLITBOUND, hasent)[bcnt]));
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
                //                std::cout << bucket->first(SPLITBOUND, i) << std::endl;
                //                std::cout << i << " NFIRST " << node->_data->first(node->_count, j) << std::endl;

            }

            bcnt += bytes(lengths[i]);

        }

        if (hasent) {
            I* ents = bucket->entries(SPLITBOUND, true);
            for (size_t i = 0; i < SPLITBOUND; ++i) {
                entry_t& ent = _entries->operator[](ents[i]);
                if(sizeof(I) == sizeof(size_t)) ent.node = (size_t)fwd_n;
                else ent.node = fwd_n->_index;
                ent.path = (bucket->first(SPLITBOUND, i));
            }
        }

        if (lcnt == 0) {
            if (hasent)
                memcpy(node->_data->entries(SPLITBOUND, true), bucket->entries(SPLITBOUND, true), SPLITBOUND * sizeof (I));
            free(bucket);
            //            std::cout << "SPLIT ALL HIGH" << std::endl;
            low_n->_data = NULL;

            split_node(node, thread, fwd_n, locked, bsize > 0 ? bsize - 1 : 0, byte + 1);
        }
        else if (hcnt == 0) {
            if (hasent)
                memcpy(low_n->_data->entries(SPLITBOUND, true), bucket->entries(SPLITBOUND, true), SPLITBOUND * sizeof (I));

            free(bucket);
            //            std::cout << "SPLIT ALL LOW" << std::endl;
            node->_data = NULL;
            split_node(low_n, thread, fwd_n, NULL, bsize > 0 ? bsize - 1 : 0, byte + 1);
        } else {
            if (hasent) {
                // We are stopping splitting here, so correct entries if needed
                I* ents = bucket->entries(SPLITBOUND, true);

                for (size_t i = 0; i < SPLITBOUND; ++i) {
                    if (i < low_n->_count) low_n->_data->entries(low_n->_count, true)[i] = ents[i];
                    else node->_data->entries(node->_count, true)[i - low_n->_count] = ents[i];
                }
            }
            free(bucket);
            //            std::cout << "SPLIT Moving LOW " << low_n->_count << " TO " << low_n << std::endl;
            //            std::cout << "SPLIT Moving HIGH " << node->_count << " TO " << node << std::endl;

        }
    }

    template<PTRIETPL>
    void set<PTRIEDEF>::split_node(node_t* node, size_t thread, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte) {

        assert(bsize != std::numeric_limits<size_t>::max());
        if (node->_type == 8) // new fwd node!
        {
            //stuff
            split_fwd(node, thread, jumppar, locked, bsize, byte);
            return;
        }

#ifndef NDEBUG
        size_t bucketsize = node->_count;
#endif
        //        assert(bucketsize <= 255);

        node_t* h_node = new_node(thread);


        //        std::cerr << "Split " << (locked ? locked : node) << " high : " << h_node << std::endl;

        h_node->_type = node->_type + 1;
        assert(h_node->_type <= 8);
        assert(node->_type <= 8);

        //assert(n_node->_lck == 0);
        h_node->_path = node->_path | binarywrapper_t::_masks[node->_type];

        // because we are only really shifting around bits when enc_pos % 8 = 0
        // then we need to find out which bit of the first 8 we are
        // splitting on in the "remainder"
        const uint r_pos = node->_type;
        assert(r_pos != 0 && r_pos < 8);

        // Copy over the data to the new buckets

        int lcnt = 0;
        int hcnt = SPLITBOUND;
        int lsize = 0;

#ifndef NDEBUG
        bool high = false;
#endif

        for (size_t i = 0; i < SPLITBOUND; i++) {

            uchar* f = (uchar*) & node->_data->first(SPLITBOUND, i);
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
        h_node->_count = hcnt;
        h_node->_totsize = node->_totsize - lsize;
        node->_count = lcnt;
        node->_totsize = lsize;

        size_t b = h_node->_path;
        size_t e = h_node->_path;
        for (int i = h_node->_type; i < 8; ++i) {
            e |= binarywrapper_t::_masks[i];
        }

        assert(b <= e);
        do {
            (*jumppar)[b] = h_node;
            ++b;
        } while (b <= e);

        node->_type += 1;

        const bool hasent = _entries != NULL;

        if (node->_count == 0) // only high node has data
        {
            //            std::cout << "ALL HIGH" << std::endl;
            node->_data = NULL;
            h_node->_data = old;
            split_node(h_node, thread, jumppar, NULL, bsize, byte);
        } else if (h_node->_count == 0) // only low node has data
        {
            //            std::cout << "ALL LOW" << std::endl;
            h_node->_data = NULL;
            node->_data = old;
            split_node(node, thread, jumppar, locked, bsize, byte);
        } else {
            //            std::cout << "Moving LOW " << node->_count << " TO " << node << std::endl;
            //            std::cout << "Moving HIGH " << h_node->_count << " TO " << h_node << std::endl;
            h_node->_data = (bucket_t*) malloc(h_node->_totsize + 
                    bucket_t::overhead(h_node->_count, hasent));
            node->_data = (bucket_t*) malloc(node->_totsize + 
                    bucket_t::overhead(node->_count, hasent));

            // copy firsts
            memcpy(&node->_data->first(node->_count), &(old->first(SPLITBOUND)), sizeof (uint16_t) * node->_count);
            memcpy(&h_node->_data->first(h_node->_count), &(old->first(SPLITBOUND, node->_count)), sizeof (uint16_t) * h_node->_count);

            // copy data
            memcpy(node->_data->data(node->_count, hasent), old->data(SPLITBOUND, hasent), node->_totsize);
            memcpy(h_node->_data->data(h_node->_count, hasent), &(old->data(SPLITBOUND, hasent)[node->_totsize]), h_node->_totsize);

            if (hasent) {
                I* ents = old->entries(SPLITBOUND, true);

                for (size_t i = 0; i < SPLITBOUND; ++i) {
                    if (i < node->_count) node->_data->entries(node->_count, true)[i] = ents[i];
                    else h_node->_data->entries(h_node->_count, true)[i - node->_count] = ents[i];
                }
            }

            free(old);
        }
    }

    template<PTRIETPL>
    std::pair<bool, size_t>
    set<PTRIEDEF>::exists(const uchar* data, uint16_t length) {
        binarywrapper_t encoding((uchar*) data, length * 8);
        //        memcpy(encoding.raw()+2, data, length);
        //        length += 2;
        //        memcpy(encoding.raw(), &length, 2);

        uint b_index = 0;

        fwdnode_t* fwd = _root;
        node_t* node = NULL;
        uint byte = 0;

        b_index = 0;
        bool res = best_match(encoding, &fwd, &node, byte, b_index);

        returntype_t ret = returntype_t(res, std::numeric_limits<size_t>::max());
        if (_entries != NULL && res) {
            ret.second = node->_data->entries(node->_count, true)[b_index];
        }
        return ret;
    }

    template<PTRIETPL>
    returntype_t
    set<PTRIEDEF>::insert(const uchar* data, uint16_t length, size_t thread) {
        binarywrapper_t e2((uchar*) data, length * 8);
        const bool hasent = _entries != NULL;
        //        binarywrapper_t encoding(length*8+16);
        //        memcpy(encoding.raw()+2, data, length);
        //        length += 2;
        //        memcpy(encoding.raw(), &length, 2);

        uint b_index = 0;

        fwdnode_t* fwd = _root;
        node_t* node = NULL;
        uint byte = 0;

        bool res = best_match(e2, &fwd, &node, byte, b_index);
        if (res) { // We are not inserting duplicates, semantics of PTrie is a set.
            returntype_t ret(false, 0);
            if (hasent) {
                ret = returntype_t(false, node->_data->entries(node->_count, true)[b_index]);
            }
            return ret;
        }


        // shallow copy
        binarywrapper_t nenc(e2.const_raw(),
                (e2.size() - byte)*8,
                byte * 8,
                e2.size() * 8);

        //        std::cout << "COULD NOT FIND of size " << e2.size() << std::endl;
        //        e2.print();
        //        std::cout << "Inserted into " << node << std::endl;

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

            entry = nbucket->entries(nbucketcount, true)[b_index] = _entries->next(thread);
            entry_t& ent = _entries->operator[](entry);
            if(sizeof(I) == sizeof(size_t)) ent.node = (size_t)fwd;
            else ent.node = fwd->_index;
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
        if (node->_count == SPLITBOUND) {
            // copy over data to we can work while readers finish
            // we have to wait for readers to finish for 
            // tree extension
            split_node(node, thread, fwd, node, nenc.size(), byte);
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
    set<PTRIEDEF>::insert(binarywrapper_t wrapper, size_t thread) {
        return insert(wrapper.raw(), wrapper.size(), thread);
    }

    template<PTRIETPL>
    returntype_t
    set<PTRIEDEF>::exists(binarywrapper_t wrapper, size_t thread) {
        return exists(wrapper.raw(), wrapper.size(), thread);
    }

}



#endif /* PTRIE_H */
