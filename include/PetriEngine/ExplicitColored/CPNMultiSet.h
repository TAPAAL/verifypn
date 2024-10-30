#ifndef MULTI_SET_H
#define MULTI_SET_H

#include "AtomicTypes.h" 
#include <vector>
#include <map>
#include <utils/errors.h>

namespace PetriEngine
{
    namespace ExplicitColored
    {
        class ColorSequence {
        public:
            ColorSequence(const ColorSequence&) = default;
            ColorSequence(ColorSequence&&) = default;
            ColorSequence& operator=(const ColorSequence&) = default;
            ColorSequence& operator=(ColorSequence&&) = default;
            explicit ColorSequence(std::vector<Color_t> colorSequence)
                : _sequence(std::move(colorSequence)) {}

            bool operator<(const ColorSequence& other) const {
                if (_sequence.size() != other._sequence.size()) {
                    throw base_error("Cannot compare sequences with inconsistent cardinalities");
                }
                for (size_t i = 0; i < _sequence.size(); i++) {
                    if (_sequence[i] < other._sequence[i]) {
                        return true;
                    }

                    if (_sequence[i] > other._sequence[i]) {
                        return false;
                    }
                }
                return false;
            }

            const std::vector<Color_t>& getSequence() const {
                return _sequence;
            }
        private:
            std::vector<Color_t> _sequence;
        };
        struct CPNMultiSet
        {
            CPNMultiSet() = default;
            CPNMultiSet(const CPNMultiSet& multiSet) {
                    _counts = multiSet._counts;
                    _cardinality = multiSet._cardinality;
            };
            CPNMultiSet(const std::map<ColorSequence, MarkingCount_t>& map, uint32_t cardinality) {
                _counts = map;
                _cardinality = cardinality;
            };
            CPNMultiSet(CPNMultiSet&&) = default;
            CPNMultiSet& operator=(const CPNMultiSet& other) {
                   _counts = other._counts;
                   _cardinality = other._cardinality;
                return *this;
            };
            CPNMultiSet& operator=(CPNMultiSet&&) = default;

            MarkingCount_t getCount(const ColorSequence& color) const;
            void setCount(const ColorSequence& color, MarkingCount_t count);
            MarkingCount_t totalCount() const;
            CPNMultiSet& operator+=(const CPNMultiSet& other);
            CPNMultiSet& operator-=(const CPNMultiSet& other);
            CPNMultiSet& operator*=(MarkingCount_t scalar);
            bool operator==(const CPNMultiSet& other) const;
            bool operator>=(const CPNMultiSet& other) const;
            bool operator<=(const CPNMultiSet& other) const;

            void stableEncode(std::ostream& out) const;
        private:
            std::map<ColorSequence, MarkingCount_t> _counts = std::map<ColorSequence, MarkingCount_t>{};
            MarkingCount_t _cardinality = 0;
        };
    }
}

#endif