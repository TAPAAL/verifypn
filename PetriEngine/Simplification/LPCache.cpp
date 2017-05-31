/* 
 * File:   LPFactory.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 31 May 2017, 09:26
 */

#include "LPCache.h"
#include "LinearProgram.h"


namespace PetriEngine {
    namespace Simplification {
        
            
        LPCache::LPCache() {
        }


        LPCache::~LPCache() {
        }
        
        LinearProgram* LPCache::cacheProgram(LinearProgram&& program)
        {
            auto res = programs.insert(program);
            LinearProgram& p = const_cast<LinearProgram&>(*res.first);
            p.inc();            
            assert(p.refs() == 0);
            return &p;
        }
        
        void LPCache::invalidate(LinearProgram* lp)
        {
            assert(lp->refs() == 0);
        }
        
    }
}

