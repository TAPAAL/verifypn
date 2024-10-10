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
            int getCount(const std::vector<Color_t>& color);
            CPNMultiSet operator+=(const CPNMultiSet& other);
            CPNMultiSet operator-=(const CPNMultiSet& other);
            CPNMultiSet operator*=(MarkingCount_t scalar);
            void SetCount(const std::vector<Color_t>& color, MarkingCount_t count);
        private:
            std::vector<MarkingCount_t> _counts;
        };
    }
}
#endif