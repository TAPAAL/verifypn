/* 
 * File:   ptrie.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 18:44
 */

#ifndef PTRIE_H
#define	PTRIE_H


#include <assert.h>
#include <limits>
#include <vector>
#include <stdint.h>
#include <stack>
#include <string.h>
#include <algorithm>

#include "visitor.h"
#include "Memory/pool_dynamic_allocator.h"
#include "Memory/pool_fixed_allocator.h"



namespace ptrie
{
    typedef uint32_t uint;
    typedef unsigned char uchar;
    
    template<typename W, typename T>
    class ptrie_t;
    
    template<typename W, typename T>
    class ptriepointer_t
    {
        typedef W encoding_t;
        public:
            ptrie_t<W, T>* container;
            uint index;
        public:
            ptriepointer_t(ptrie_t<W, T>* container, uint i); 
            T& get_meta() const;
            void set_meta(T);
            uint write_partial_encoding(encoding_t&) const;
            encoding_t& remainder() const;
            
            ptriepointer_t() : container(NULL), index(0) {};
            
            ptriepointer_t<W, T>& operator=(const ptriepointer_t<W, T>&);
            ptriepointer_t<W, T>& operator++();
            ptriepointer_t<W, T>& operator--();
            bool operator==(const ptriepointer_t<W, T>& other);
            bool operator!=(const ptriepointer_t<W, T>& other);
            
    };
    
    template<typename W, typename T>
    ptriepointer_t<W, T>::ptriepointer_t(ptrie_t<W, T>* container, uint i) : 
    container(container), index(i)
    {        
    }
    
    template<typename W, typename T>

    ptriepointer_t<W, T>& ptriepointer_t<W, T>::operator=
                                                (const ptriepointer_t<W, T>& other)
    {
        container = other.container;
        index = other.index;
        return *this;
    }
    
    template<typename W, typename T>
    ptriepointer_t<W, T>& ptriepointer_t<W, T>::operator++()
    {
        ++index;
        assert(index <= container->_next_free_entry);
        return *this;
    }
    
    template<typename W, typename T>
    ptriepointer_t<W, T>& ptriepointer_t<W, T>::operator--()
    {
        --index;
        return *this;
    }
    
    template<typename W, typename T>
    bool ptriepointer_t<W, T>::operator==(const ptriepointer_t<W, T>& other)
    {
        return other.container == container && other.index == index;
    }
    
    template<typename W, typename T>
    bool ptriepointer_t<W, T>::operator!=(const ptriepointer_t<W, T>& other)
    {
        return !(*this == other);
    }
    
    /*
    template<typename T>
    void swap(ptriepointer<T>& lhs, ptriepointer<T>& rhs)
    {
        ptrie<T>* container = lhs.container;
        uint index = lhs.index;
        lhs.container = rhs.container;
        lhs.index = rhs.index;
        rhs.index = index;
        rhs.container = container;
    }*/
    
    template<typename W, typename T>
    T &ptriepointer_t<W, T>::get_meta() const
    {
        return container->get_entry(index)->_data.get_meta();
    }
    
    template<typename W, typename T>
    void ptriepointer_t<W, T>::set_meta(T val)
    {
        container->get_entry(index)->_data.set_meta(val);
    }
    
    template<typename W, typename T>
    uint ptriepointer_t<W, T>::write_partial_encoding(encoding_t& encoding) const
    {
        return container->writePathToRoot(index, encoding);
    }
    
    template<typename W, typename T>
    W& ptriepointer_t<W, T>::remainder() const
    {
        return container->get_entry(index)->_data;
    }
    
    template<typename W, typename T>
    class ptrie_t {
        typedef W encoding_t;  
        friend class ptriepointer_t<W, T>;

        private:
            
            /*
            typing scheme
                nnnxxxqq
                n=bit num mod 8
                x=2^x children
                q=currently unused
             */
                        
            template<uchar width>
            struct jumpnode_t
            {
                uint _positions[width];
                uint _parent;
                uchar _type;
                uchar _path;
            } __attribute__((packed));
            
            // nodes in the tree
            struct node_t : public jumpnode_t<2>
            {
                char _count[2];      // bucket-counts
                uint* _entries;          // back-pointers to data-array up to date
            } __attribute__((packed));
            
            struct entry_t
            {
                encoding_t _data;       // remainder-data, not in path of tree 
                uint _nodeindex;        // index of node
            };
            
            const uint _blocksize;   // number to allocate at a time
            uint _next_free_node;      // location of next vacant node-location
            uint _next_free_entry;     // location of next vacant entry-node
            
            std::vector<node_t*> _nodevector;    // Vector of arrays of nodes in tree
            std::vector<entry_t*> _entryvector;  // Vector of remainders
            
            short int _threshold;
            
            uint* buffer;
            size_t buffersize;
            
            pdalloc::pool_dynamic_allocator<uint> ballocator;
            pdalloc::pool_dynamic_allocator<unsigned char> dallocator;
            palloc::pool_fixed_allocator<uint[256]> fwdalloc;
            
        protected:
            node_t* get_node(uint index) const;
            uint new_node();
            
            entry_t* get_entry(uint index) const;
            uint new_entry();
            
            bool best_match(const encoding_t& encoding, uint& tree_pos , uint& e_index,
                                uint& enc_pos, uint& b_index, uint& bucketsize);
            
            uint split_node(node_t* node, uint tree_pos, uint enc_pos, 
                    uint bucketsize, bool branch, uint* lookup);

            uint writePathToRoot(uint n_index, encoding_t& encoding) const;
            
        public:
            typedef ptriepointer_t<W, T> pointer_t;
            ptrie_t();
            ~ptrie_t();

            std::pair<bool, pointer_t > insert(const encoding_t& encoding);
            std::pair<bool, pointer_t > find(const encoding_t& encoding);
            bool consistent() const;
            uint size() const { return _next_free_entry; }
            pointer_t begin();
            pointer_t end();
            pointer_t last();
            pointer_t rend();
            
            void visit(visitor_t<W, T>&);
    };
    
    template<typename W, typename T>
    ptrie_t<W, T>::~ptrie_t() {
        std::cout << "Nodes : " << _next_free_node << std::endl;
        for(size_t i = 0; i < _nodevector.size(); ++i)
        {
            delete[] _nodevector[i];
        }
        size_t deleted = 0;
        _nodevector.clear();
        for(size_t i = 0; i < _entryvector.size(); ++i)
        {
            delete[] _entryvector[i];
        }
        _entryvector.clear();
        ballocator.deallocate(buffer, buffersize);

    }
    
    template<typename W, typename T>
    ptrie_t<W, T>::ptrie_t() :
            _blocksize(1024*1024), 
            _next_free_node(1), 
            _next_free_entry(0), 
            _nodevector(), 
            _entryvector(), 
            _threshold(std::min((size_t)8 * sizeof(node_t), (size_t) 126))
    {
        _nodevector.push_back(new node_t[_blocksize]);
        _nodevector[0][0]._entries = NULL;
        _nodevector[0][0]._count[1] = 0;
        _nodevector[0][0]._count[0] = 0;
        _nodevector[0][0]._positions[1] = 0;
        _nodevector[0][0]._positions[0] = 0;
        _nodevector[0][0]._parent = 0;
        _nodevector[0][0]._type = 1;
        _nodevector[0][0]._path = 0;
        
        uint* lu = (uint*)fwdalloc.allocate(1);
        memset(lu, 0, sizeof(uint)*256);
        split_node(&_nodevector[0][0], 0, 1, 0, true, lu);
        split_node(&_nodevector[0][0], 0, 1, 0, false, lu);
        
        _nodevector[0][0]._entries = lu;
        
        buffersize = 1024;
        buffer = ballocator.allocate(buffersize);
    }
    
    template<typename W, typename T>
    typename ptrie_t<W,T>::pointer_t ptrie_t<W,T>::begin()
    {
        return pointer_t(this, 0);
    }
    
    template<typename W, typename T>
    typename ptrie_t<W,T>::pointer_t ptrie_t<W, T>::end()
    {
        return pointer_t(this, _next_free_entry);
    }
    
    template<typename W, typename T>
    typename ptrie_t<W,T>::pointer_t ptrie_t<W,T>::last()
    {
        return pointer_t(this, _next_free_entry-1);
    }
    
    template<typename W, typename T>
    typename ptrie_t<W, T>::pointer_t ptrie_t<W, T>::rend()
    {
        return pointer_t(this, std::numeric_limits<uint>::max());
    }
    
    template<typename W, typename T>
    typename ptrie_t<W,T>::node_t*
    ptrie_t<W,T>::get_node(uint index) const
    {
        assert(index < _next_free_node);
        return &_nodevector[index / _blocksize][index % _blocksize];
    }
    
    template<typename W, typename T>
    uint ptrie_t<W,T>::new_node()
    {
        uint next = _next_free_node;
        if(next % _blocksize == 0)
        {
            _nodevector.push_back(new node_t[_blocksize]);
        }
        
        ++_next_free_node;
        return next;
    }
    
    template<typename W, typename T>
    typename ptrie_t<W,T>::entry_t* 
    ptrie_t<W,T>::get_entry(uint index) const
    {
        assert(index < _next_free_entry);
        return &_entryvector[index / _blocksize][index % _blocksize];
    }
    
    template<typename W, typename T>
    uint ptrie_t<W,T>::new_entry()
    {
        uint next = _next_free_entry;
        if(next % _blocksize == 0)
        {
            _entryvector.push_back(new entry_t[_blocksize]);
        }
        
        ++_next_free_entry;
        return next;
    }
    
    template<typename W, typename T>
    bool ptrie_t<W,T>::consistent() const
    {
        return true;
        assert(_next_free_node >= _nodevector.size());
        assert(_next_free_entry >= _entryvector.size());
        for(size_t i = 0; i < _next_free_node; ++i)
        {
            node_t* node = get_node(i);
            assert(node->_positions[1] < _next_free_node);
            assert(node->_positions[0] < _next_free_node);
            assert(node->_parent < _next_free_node);
            assert(node->_parent == 0 && i == 0 || node->_parent < i);
            assert((node->_positions[1] > i && node->_count[1] == -1) 
                    || (node->_positions[1] == 0 && node->_count[1] >= 0));
            assert((node->_positions[0] > i && node->_count[0] == -1) 
                    || (node->_positions[0] == 0 && node->_count[0] >= 0));
            
            size_t bucket = 0;
            
            if(node->_count[1] > 0)
                bucket += node->_count[1];
            if(node->_count[0] > 0)
                bucket += node->_count[0];

            assert(node->_entries != NULL || bucket == 0);
            for(size_t e = 0; e < bucket; ++e)
            {
                assert(e < _next_free_entry);
                assert(get_entry(node->_entries[e])->_nodeindex == i);
            }
        }
        return true;
    }
    
    template<typename W, typename T>
    uint ptrie_t<W,T>::writePathToRoot(uint e_index, encoding_t& encoding) const
    {
        entry_t* ent = get_entry(e_index);
        size_t count = 0;
        uint c_index = ent->_nodeindex;
        node_t* node = get_node(c_index);
        assert(consistent());
        while(c_index != 0)
        {
            uint p_index = node->_parent;
            if(node->_type)
            {
                uchar pchar = node->_path;
                for(size_t i = 0; i < 8; ++i)
                {
                    encoding.set(count, (pchar & 0x80)); 
                    ++count;
                    pchar <<= 1;
                }
                node = get_node(p_index);
            } 
            else
            {
                node = get_node(p_index);
                bool branch = c_index == node->_positions[1];
                encoding.set(count, branch);
                ++count;
            }
            c_index = p_index;
        }
        assert(consistent());
        return count;
    }
    
    template<typename W, typename T>
    void ptrie_t<W,T>::visit(visitor_t<W, T>& visitor)
    {
        std::stack<std::pair<uint, uint> > waiting;
        waiting.push(std::pair<int, uint>(-1, 0));

        bool stop = false;
        do{
            int distance = waiting.top().first;
            uint n_index = waiting.top().second;
            waiting.pop();
            
            if(distance > -1)
            {
                visitor.back(distance);
                visitor.set(distance, true);    // only high on stack
            }
            
            node_t* node = get_node(n_index);
            bool skip = false;
            do
            {

                if(node->_positions[1] != 0)
                {
                    waiting.push(std::pair<uint, uint>(distance + 1, node->_positions[1]));
                }                 
                
                if(node->_positions[0] == 0) break;
                else
                {
                    ++distance;
                    node = get_node(node->_positions[0]);
                    skip = visitor.set(distance, false);
                }
                
            } while(!skip);
            
            distance += 1;
            
            uint bucketsize = 0;
            if(!skip)
            {
                if(node->_count[1] > 0) bucketsize += node->_count[1];
                if(node->_count[0] > 0) bucketsize += node->_count[0];
                
                for(uint i = 0; i < bucketsize && !stop; ++i)
                {
                    stop = visitor.set_remainder(distance,
                                    pointer_t(this, node->_entries[i]));
                }
            }            
        } while(!waiting.empty() && !stop);
    }
    
    template<typename W, typename T>
    bool ptrie_t<W,T>::best_match(const encoding_t& encoding, uint& tree_pos, 
                uint& e_index, uint& enc_pos, uint& b_index, uint& bucketsize)
    {
        assert(consistent());
        uint t_pos = 0;
        tree_pos = 0;
        enc_pos = 0;
        bucketsize = 0;

        size_t encsize = encoding.size() * 8;
        
        // run through tree as long as there are branches covering some of 
        // the encoding
        
        size_t nbyte = 0;
        do {
           tree_pos = t_pos;
           t_pos = get_node(t_pos)->_entries[encoding.const_raw()[nbyte]];
           if(t_pos == 0)
           {
               t_pos = tree_pos;
               break;
           }
           else
           {
               enc_pos += 8;
               ++nbyte;
           }
        } while(t_pos != 0);
         
        do {
            tree_pos = t_pos;
            if (encoding.at(enc_pos)) {
                t_pos = get_node(t_pos)->_positions[1];
            } else {
                t_pos = get_node(t_pos)->_positions[0];
            }

            assert(t_pos == 0 || get_node(t_pos)->_type || get_node(t_pos)->_parent == tree_pos);
            
            enc_pos++;

        } while (t_pos != 0);
        
        enc_pos--;  // previous always overshoots by 1
        
        assert(tree_pos != 0 || enc_pos == 0);
        
        node_t* node = get_node(tree_pos);
        
        // find out the size of the bucket
        if (node->_count[1] > 0) {
            bucketsize += node->_count[1];
            
        }
        
        if (node->_count[0] > 0) {
            bucketsize += node->_count[0];
        }
        
        // run through the stored data in the bucket, looking for matches
        // start by creating an encoding that "points" to the "unmatched"
        // part of the encoding. Notice, this is a shallow copy, no actual
        // heap-allocation happens!
        encoding_t s_enc = encoding_t(  encoding.const_raw(), 
                                        (encsize - enc_pos), 
                                        enc_pos, 
                                        encsize);
        
        bool found = false;

        e_index = std::numeric_limits<uint>::max();
        if(bucketsize > 0)
        {
            int low = 0;
            int high = bucketsize - 1;
            do
            {
                b_index = (high + low) / 2 ;
                entry_t* ent = get_entry(node->_entries[b_index]);
                
                int cmp = s_enc.cmp(ent->_data);
                
                if(cmp == 0)
                {
                    found = true;
                    e_index = node->_entries[b_index];
                    break;
                }
                else if(cmp > 0)
                {
                    low = b_index + 1;
                }
                else //if cmp < 0
                {
                    high = b_index - 1;
                }
                
                if(low > high)
                {
                    if(cmp > 0)
                        b_index += 1;
                    break;
                }
                assert(b_index < bucketsize);
            } while(true);
        }
        else
        {
            b_index = 0;
        }
         
        /*int tmp;// refference debug code!
        for(tmp = 0; tmp < bucketsize; ++tmp)
        {
            entry_t* ent = get_entry(node->_entries[tmp]);
            if(ent->_data.cmp(s_enc) < 0)
            {
                continue;
            }
            else
            if(ent->_data == s_enc)
            {
                assert(found);
                break;
            }
            else
            {
                break;
            }
        }
        assert(tmp == b_index);*/
        
        assert(consistent());        
        return found;
    }
    
    template<typename W, typename T>
    uint ptrie_t<W,T>::split_node(node_t* node, uint tree_pos, uint enc_pos, 
                                                uint bucketsize, bool branch,
                                                uint* lookup)
    {
        assert(consistent());
        uint n_node_index = new_node();
        node_t* n_node = get_node(n_node_index);
        uint* o_entries = NULL;

        // if we are overflowing in, split bucket
        uint n_node_count;
        uint node_count = 0;

        if (branch) {
            n_node_count = node->_count[1];
            n_node->_entries = ballocator.allocate(node->_count[1]);

            node->_positions[1] = n_node_index;
            node->_count[1] = -1;

            if (node->_count[0] > 0)
            {
                node_count = node->_count[0];
                o_entries = ballocator.allocate(node->_count[0]);
            }

        } else {
            n_node_count = node->_count[0];
            n_node->_entries = ballocator.allocate(node->_count[0]);
            
            node->_positions[0] = n_node_index;
            node->_count[0] = -1;

            if (node->_count[1] > 0)
            {
                node_count = node->_count[1];
                o_entries = ballocator.allocate(node->_count[1]);
            }
        }

        assert(o_entries != NULL || node_count == 0);
//        assert(n_node_count > 0);
        assert(tree_pos != n_node_index);
        n_node->_parent = tree_pos;

        n_node->_count[0] = n_node->_count[1] = 0;
        n_node->_positions[0] = n_node->_positions[1] = 0;

        // because we are only really shifting around bits when enc_pos % 8 = 0
        // then we need to find out which bit of the first 8 we are
        // splitting on in the "remainder"
        const uint r_pos = enc_pos % 8;


        size_t clistcount = 0;
        size_t nlistcount = 0;


        if ((r_pos + 1) == 8) { 
            size_t tmp_parent = n_node->_parent;
            n_node->_type = 1;
            
            for(size_t i = 0; i < 7; ++i)
                tmp_parent = get_node(tmp_parent)->_parent;
            
            n_node->_parent = tmp_parent;
        }
        else
        {
            n_node->_type = 0;
        }

        
        // Copy over the data to the new buckets
        for (size_t i = 0; i < bucketsize; i++) {
            entry_t* entry = get_entry(node->_entries[i]);
            if (entry->_data.at(r_pos) == branch) {
                // Adjust counters
                if (entry->_data.at(r_pos + 1)) {
                    n_node->_count[1]++;
                } else {
                    n_node->_count[0]++;
                }

                // This goes to the new bucket, we can maybe remove a byte
                if ((r_pos + 1) == 8) { 
                    // Tree tree is representing the first byte, remove it 
                    if(entry->_data.size() > 0)
                    {
                        n_node->_path = entry->_data.raw()[0];
                        lookup[n_node->_path] = n_node_index;
                    }
                    entry->_data.pop_front(1, dallocator);
                }

                assert(clistcount < n_node_count);
                n_node->_entries[clistcount] = node->_entries[i];
                entry->_nodeindex = n_node_index;
                clistcount++;
            } else {
                assert(nlistcount < node_count);
                o_entries[nlistcount] = node->_entries[i];
                nlistcount++;
            }
        }
        
        assert(clistcount == n_node_count);
        assert(nlistcount == node_count);

        ballocator.deallocate(node->_entries, bucketsize);
        node->_entries = o_entries; 

        if (node->_count[1] == -1 && node->_count[0] == -1) {
            assert(node->_entries == NULL);
        }
                
        if(n_node->_type == 1)
        {
            lookup = (uint*)fwdalloc.allocate(1);
            memset(lookup, 0, sizeof(uint)*256);
        }
        
        if( n_node->_count[0] > _threshold  ||
            (n_node->_type == 1 && n_node->_count[0] != -1 ) )
        {
            split_node(n_node, n_node_index, enc_pos+1, clistcount, false, lookup);
        }

        
        if( n_node->_count[1] > _threshold ||
            (n_node->_type == 1 && n_node->_count[1] != -1 ) )
        {
            size_t cnt = n_node->_count[1];
            if(n_node->_count[0] > 0) cnt += n_node->_count[0];
            split_node(n_node, n_node_index, enc_pos+1, cnt, true, lookup);
        }

        if(n_node->_type == 1)
        {
            n_node->_entries = lookup;
        }
        
        assert(consistent());
        return n_node_index;
    }
    
    template<typename W, typename T>
    std::pair<bool, typename ptrie_t<W,T>::pointer_t > 
    ptrie_t<W,T>::find(const encoding_t& encoding)
    {
        uint tree_pos;
        uint enc_pos;
        uint bucketsize;
        uint e_index;
        uint b_index;
        if(best_match(encoding, tree_pos, e_index, enc_pos, b_index,  bucketsize))
        {
            return std::pair<bool, pointer_t >(true, pointer_t(this, e_index));
        }
        else
        {
            return std::pair<bool, pointer_t >(false, pointer_t(this, e_index));
        }
    }
    
    template<typename W, typename T>
    std::pair<bool, typename ptrie_t<W,T>::pointer_t > 
    ptrie_t<W,T>::insert(const encoding_t& encoding)
    {
        assert(consistent());
        size_t encsize = encoding.size() * 8;
        uint tree_pos;
        uint enc_pos;
        uint bucketsize;
        uint e_index;
        uint b_index;
                
        if(best_match(encoding, tree_pos, e_index, enc_pos, b_index, bucketsize))
        {   // We are not inserting duplicates, semantics of PTrie is a set.
            assert(consistent());
            return std::pair<bool, pointer_t >(false, pointer_t(this, e_index));
        }

        // closest matched node
        node_t* node = get_node(tree_pos);
        
        // Else we need to insert it, start by doing a deep-copy, and putting
        // it into a new entry
        uint ne_index = new_entry();
        entry_t* n_entry = get_entry(ne_index);
        uint remainder_size = (encsize - enc_pos);
        n_entry->_data = encoding_t(  encoding, 
                                    remainder_size, 
                                    enc_pos, 
                                    encsize, dallocator);
        //assert(n_entry->_data.const_raw() != encoding.const_raw());
                
        n_entry->_nodeindex = tree_pos;
        
        // make a new bucket, add new entry to end, copy over old data
        uint oldbucketsize = bucketsize;
        bucketsize += 1;
        node->_entries = ballocator.reallocate(node->_entries, bucketsize - 1 , bucketsize);
        
        if(b_index == oldbucketsize)
        {
            node->_entries[b_index] = ne_index;
        }
        else
        {
            if(bucketsize > buffersize)
            {

                buffer = ballocator.reallocate(buffer, buffersize, buffersize);
                buffersize = bucketsize;
            }
            memcpy(buffer, &(node->_entries[b_index]), sizeof(uint)*(oldbucketsize - b_index));
            node->_entries[b_index] = ne_index;
            memcpy(&(node->_entries[b_index+1]), buffer, sizeof(uint)*(oldbucketsize - b_index));
        }
        
        // increment correct counter
        short int count;
        bool branch = encoding.at(enc_pos);
        if (branch) {
            count = (++node->_count[1]);
            assert(node->_count[1] <= 127);
            assert(node->_positions[1] == 0);
        } else {
            count = (++node->_count[0]);
            assert(node->_count[1] <= 127);
            assert(node->_positions[0] == 0);
        }
        
        // if needed, split the node
        if(count > _threshold)
        {
            node_t* tmp_parent = node;
            while(tmp_parent->_type != 1)
                tmp_parent = get_node(tmp_parent->_parent);
            split_node(node, tree_pos, enc_pos, bucketsize, branch, tmp_parent->_entries);
        }
        assert(consistent());
        return std::pair<bool, pointer_t >(true, pointer_t(this, ne_index));
    }
}



#endif	/* PTRIE_H */
