//
// Created by joms on 11/29/24.
//

#ifndef SEQUENCEMULTISET_H
#define SEQUENCEMULTISET_H
#include <algorithm>

namespace PetriEngine {
    namespace ExplicitColored {
        template<typename K>
        class SequenceMultiSet {
        public:
            SequenceMultiSet() = default;
            SequenceMultiSet(const SequenceMultiSet&) = default;
            SequenceMultiSet& operator=(const SequenceMultiSet&) = default;
            SequenceMultiSet(SequenceMultiSet&&) = default;
            SequenceMultiSet& operator=(SequenceMultiSet&&) = default;

            MarkingCount_t getCount(const K& color) const {
                auto& it = std::lower_bound(_counts.first(), _counts.end(), color);
                if (it != _counts.end() && it->first == color) {
                    return it->second;
                }
                return 0;
            }

            void setCount(const K& color, MarkingCount_t count) {
                auto& it = std::lower_bound(_counts.first(), _counts.end(), color);
                if (it != _counts.end() && it->first == color) {
                    it->second = count;
                    return;
                }
                _counts.insert(it, {color, count});
            }

            void addCount(const K& color, int32_t count) {
                auto& it = std::lower_bound(_counts.first(), _counts.end(), color);
                if (it != _counts.end() && it->first == color) {
                    if (it->second + count < 0) {
                        _cardinality -= it->second;
                        it->second = 0;
                    } else {
                        it->second += count;
                        _cardinality += count;
                    }
                    return;
                }
                _counts.insert(it, {color, count});
            }

            MarkingCount_t totalCount() const {
                return _cardinality;
            }

            SequenceMultiSet& operator+=(const SequenceMultiSet& other) {
                auto aIt = _counts.begin();
                auto bIt = other._counts.begin();
                while (aIt != _counts.end() && bIt != other._counts.end()) {
                    if (aIt->first == bIt->first) {
                        aIt->second += bIt->second;
                    } else if (aIt->first > bIt->first) {
                        ++bIt;
                    } else {
                        ++aIt;
                    }
                }

                return *this;
            }

            SequenceMultiSet& operator-=(const SequenceMultiSet& other);
            SequenceMultiSet& operator*=(MarkingCount_t scalar);
            bool operator==(const SequenceMultiSet& other) const;
            bool operator>=(const SequenceMultiSet& other) const;
            bool operator<=(const SequenceMultiSet& other) const;
        private:
            std::vector<std::pair<K, MarkingCount_t>> _counts;
            MarkingCount_t _cardinality;
        };
    }
}

#endif //SEQUENCEMULTISET_H
