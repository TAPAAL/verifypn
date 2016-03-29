/* 
 * File:   visitor.h
 * Author: Peter G. Jensen
 *
 * Created on 16 June 2015, 23:15
 */

#ifndef VISITOR_H
#define	VISITOR_H

#include <stdint.h>

namespace ptrie
{
    template<typename W, typename T>
    class ptriepointer_t;
    
    template<typename W, typename T>
    class visitor_t
    {
    public:
        virtual bool back(int index) = 0;
        virtual bool set(int index, bool value) = 0;
        virtual bool set_remainder(int index, ptriepointer_t<W,T> pointer) = 0;
    };
}


#endif	/* VISITOR_H */

