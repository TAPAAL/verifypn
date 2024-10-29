#include <algorithm>
#include "PetriEngine/ExplicitColored/CPNMultiSet.h"
#include "utils/errors.h"
namespace PetriEngine {
    namespace ExplicitColored {
        MarkingCount_t CPNMultiSet::getCount(const ColorSequence& color) const {
            const auto colorIt = _counts.find(color);
            if (colorIt == _counts.end()) {
                return 0;
            }
            return colorIt->second;
        }
        
        void CPNMultiSet::setCount(const ColorSequence& color, MarkingCount_t count) {
            auto it = _counts.find(color);
            if (it == _counts.end()) {
                _counts.emplace(color, count);
                _cardinality += count;
                return;
            }
            _cardinality = (_cardinality + count) - it->second;
            it->second = count;
        }

        CPNMultiSet& CPNMultiSet::operator+=(const CPNMultiSet& other) {
            for (auto& otherCount : other._counts) {
                auto it = _counts.find(otherCount.first);
                if (it == _counts.end()) {
                    _counts.emplace(otherCount);
                } else {
                    it->second += otherCount.second;
                }
            }
            _cardinality += other._cardinality;
            return *this;
        }

        CPNMultiSet& CPNMultiSet::operator-=(const CPNMultiSet& other) {
            for (auto& otherCount : other._counts) {
                auto it = _counts.find(otherCount.first);
                if (it == _counts.end()) {
                    continue;
                }
                if (otherCount.second > it->second) {
                    _cardinality -= it->second;
                    it->second = 0;
                } else {
                    _cardinality -= otherCount.second;
                    it->second -= otherCount.second;
                }
            }
            return *this;
        }

        CPNMultiSet& CPNMultiSet::operator*=(MarkingCount_t scalar) {
            for (auto& colorCount : _counts) {
                colorCount.second *= scalar;
            }
            _cardinality *= scalar;
            return  *this;
        }

        MarkingCount_t CPNMultiSet::totalCount() const {
            return _cardinality;
        }

        bool CPNMultiSet::operator>=(const CPNMultiSet& other) const {
            if (_cardinality < other._cardinality)
                return false;
            return std::all_of(other._counts.begin(), other._counts.end(),[&](auto& count){
                return count.second <= getCount(count.first);
            });
        }

        bool CPNMultiSet::operator<=(const CPNMultiSet& other) const {
            if (_cardinality > other._cardinality){
                return false;
            }
            return std::all_of(other._counts.begin(), other._counts.end(),[&](auto& count){
                return count.second >= getCount(count.first);
            });
        }

        bool CPNMultiSet::operator==(const CPNMultiSet& other) const {
            if (_cardinality != other._cardinality){
                return false;
            }
            return std::all_of(other._counts.begin(), other._counts.end(),[&](auto& count){
                return count.second == getCount(count.first);
            });
        }

        void CPNMultiSet::stableEncode(std::ostream& out) const {
            for (const auto& count : _counts) {
                if (count.second == 0) {
                    continue;
                }

                out << count.second << "'" << "(";
                for (const auto& color : count.first.getSequence()) {
                    out << color << ",";
                }
                out << ")";
            }
        }
    }
}