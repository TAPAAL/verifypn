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
            for (const auto&[fst, snd] : other._counts) {
                auto it = _counts.find(fst);
                if (it == _counts.end()) {
                    continue;
                }
                if (snd > it->second) {
                    _cardinality -= it->second;
                    it->second = 0;
                } else {
                    _cardinality -= snd;
                    it->second -= snd;
                }
            }
            return *this;
        }

        CPNMultiSet& CPNMultiSet::operator*=(const MarkingCount_t scalar) {
            for (auto&[fst, snd] : _counts) {
                snd *= scalar;
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
            for (const auto& count : other._counts) {
                if (count.second > getCount(count.first))
                    return false;
            }
            return true;
        }

        bool CPNMultiSet::operator<=(const CPNMultiSet& other) const {
            if (_cardinality > other._cardinality)
                return false;
            for (const auto&[fst, snd] : other._counts) {
                if (snd < getCount(fst))
                    return false;
            }
            return true;
        }

        bool CPNMultiSet::operator==(const CPNMultiSet& other) const {
            if (_cardinality != other._cardinality){
                return false;
            }
            for (const auto&[fst, snd] : _counts) {
                auto otherCount = other.getCount(fst);
                if (otherCount != snd) {
                    return false;
                }
            }
            return true;
        }

        void CPNMultiSet::stableEncode(std::ostream& out) const {
            for (const auto&[fst, snd] : _counts) {
                if (snd == 0) {
                    continue;
                }

                out << snd << "'" << "(";
                for (const auto& color : fst.getSequence()) {
                    out << color << ",";
                }
                out << ")";
            }
        }
    }
}