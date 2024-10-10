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
            CPNMultiSet() = default;
            CPNMultiSet(const CPNMultiSet&) = default;
            CPNMultiSet(CPNMultiSet&&) = default;
            CPNMultiSet& operator=(const CPNMultiSet&) = default;
            CPNMultiSet& operator=(CPNMultiSet&&) = default;

            MarkingCount_t getCount(const std::vector<Color_t>& color) const;
            CPNMultiSet& operator+=(const CPNMultiSet& other);
            CPNMultiSet& operator-=(const CPNMultiSet& other);
            CPNMultiSet& operator*=(MarkingCount_t scalar);
            void SetCount(std::vector<Color_t> color, MarkingCount_t count);
        private:
            std::vector<std::pair<std::vector<Color_t>, MarkingCount_t>> _counts;
            MarkingCount_t _cardinality;
        };
    }
}

#endif