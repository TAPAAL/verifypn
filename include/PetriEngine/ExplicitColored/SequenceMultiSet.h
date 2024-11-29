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
                auto it = lower_bound(color);
                if (it != _counts.end() && it->first == color) {
                    return it->second;
                }
                return 0;
            }

            void setCount(const K& color, MarkingCount_t count) {
                auto it = lower_bound(color);
                if (it != _counts.end() && it->first == color) {
                    _cardinality = (_cardinality + count) - it->second;
                    it->second = count;
                    return;
                }
                _counts.insert(it, {color, count});
                _cardinality += count;
            }

            void addCount(const K& color, int32_t count) {
                auto it = lower_bound(color);
                if (it != _counts.end() && it->first == color) {
                    if (static_cast<int32_t>(it->second) + count < 0) {
                        _cardinality -= it->second;
                        it->second = 0;
                    } else {
                        it->second += count;
                        _cardinality += count;
                    }
                    return;
                }
                _counts.insert(it, {color, count});
                _cardinality += count;
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
                        _cardinality += bIt->second;
                        ++aIt;
                        ++bIt;
                    } else if (aIt->first < bIt->first) {
                        ++aIt;
                    } else {
                        aIt = _counts.insert(aIt, *bIt);
                        _cardinality += bIt->second;
                        ++aIt;
                    }
                }
                for (; bIt != other._counts.end(); ++bIt) {
                    _counts.push_back(*bIt);
                }
                return *this;
            }

            SequenceMultiSet& operator-=(const SequenceMultiSet& other) {
                auto aIt = _counts.begin();
                auto bIt = other._counts.begin();
                while (aIt != _counts.end() && bIt != other._counts.end()) {
                    if (aIt->first == bIt->first) {
                        if (bIt->second > aIt->second) {
                            _cardinality -= aIt->second;
                            aIt->second = 0;
                        } else {
                            aIt->second -= bIt->second;
                            _cardinality -= bIt->second;
                        }
                        ++aIt;
                        ++bIt;
                    } else if (aIt->first < bIt->first) {
                        ++aIt;
                    } else {
                        ++bIt;
                    }
                }
                return *this;
            }

            SequenceMultiSet& operator*=(MarkingCount_t scalar) {
                for (auto& pair : _counts) {
                    pair.second *= scalar;
                }
                return *this;
            }

            bool operator==(const SequenceMultiSet& other) const {
                if (_cardinality != other._cardinality || _counts.size() != other._counts.size()) {
                    return false;
                }
                auto aIt = _counts.begin();
                auto bIt = other._counts.begin();
                while (aIt != _counts.end() && bIt != other._counts.end()) {
                    if (aIt->first != bIt->first || aIt->second != bIt->second) {
                        return false;
                    }
                    ++aIt;
                    ++bIt;
                }

                return true;
            }

            bool operator>=(const SequenceMultiSet& other) const {
                if (_cardinality < other._cardinality || _counts.size() < other._counts.size()) {
                    return false;
                }
                auto aIt = _counts.begin();
                auto bIt = other._counts.begin();
                while (aIt != _counts.end() && bIt != other._counts.end()) {
                    if (aIt->first == bIt->first) {
                        if (aIt->second < bIt->second) {
                            return false;
                        }
                        ++aIt;
                        ++bIt;
                    } else if (aIt->first < bIt->first) {
                        ++aIt;
                    } else {
                        return false;
                    }
                }
                if (bIt != other._counts.end()) {
                    return false;
                }
                return true;
            }

            bool operator<=(const SequenceMultiSet& other) const {
                if (_cardinality > other._cardinality || _counts.size() > other._counts.size()) {
                    return false;
                }
                auto aIt = _counts.begin();
                auto bIt = other._counts.begin();
                while (aIt != _counts.end() && bIt != other._counts.end()) {
                    if (aIt->first == bIt->first) {
                        if (aIt->second > bIt->second) {
                            return false;
                        }
                        ++aIt;
                        ++bIt;
                    } else if (aIt->first < bIt->first) {
                        return false;
                    } else {
                        ++bIt;
                    }
                }

                if (aIt != _counts.end()) {
                    return false;
                }
                return true;
            }

            const std::vector<std::pair<K, MarkingCount_t>>& counts() const {
                return _counts;
            }
        private:
            typename std::vector<std::pair<K, MarkingCount_t>>::iterator lower_bound(const K& key) {
                return std::lower_bound(
                     _counts.begin(),
                     _counts.end(),
                     key,
                     [](const auto& a, const auto& b) {
                         return a.first < b;
                     }
                );
            }
            std::vector<std::pair<K, MarkingCount_t>> _counts;
            MarkingCount_t _cardinality = 0;
        };
    }
}

#endif //SEQUENCEMULTISET_H
