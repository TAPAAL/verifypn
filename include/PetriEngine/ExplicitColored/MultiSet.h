#ifndef MULTI_SET_H
#define MULTI_SET_H
#include "AtomicTypes.h"
#include <vector>
namespace PetriEngine
{
    namespace ExplicitColored
    {
        struct CPNMultiSet
        {
            u_int32_t getCount(Color_t color);
            u_int32_t totalCount();
        private:
            std::vector<MarkingCount_t> _counts;
        };
    }
}
#endif