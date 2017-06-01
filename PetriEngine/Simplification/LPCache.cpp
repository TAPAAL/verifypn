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
//            return new LinearProgram(std::move(program));
            auto res = programs.insert(program);
            LinearProgram& p = const_cast<LinearProgram&>(*res.first);
            p.inc();            
            //if(res.second && (programs.size() % 1000) == 0) std::cout << "PROGRAMS : " << programs.size() << std::endl;
//            assert(p.refs() > 0);
            return &p;
        }
        
        void LPCache::invalidate(LinearProgram* lp)
        {
//            delete lp;
//            assert(lp->refs() == 0);
        }
        
    }
}

