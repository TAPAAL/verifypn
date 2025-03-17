#ifndef SEQUENCEMULTISET_H
#define SEQUENCEMULTISET_H

#include <algorithm>
#include "ColorSequence.h"
#include "ExplicitErrors.h"

namespace PetriEngine::ExplicitColored {
    class SequenceMultiSet {
    public:
        SequenceMultiSet() = default;
        SequenceMultiSet(const SequenceMultiSet&) = default;
        SequenceMultiSet& operator=(const SequenceMultiSet&) = default;
        SequenceMultiSet(SequenceMultiSet&&) = default;
        SequenceMultiSet& operator=(SequenceMultiSet&&) = default;

        [[nodiscard]] MarkingCount_t getCount(const ColorSequence& colorSequence) const {
            const Color_t& color = colorSequence.encodedValue;
            const auto it = clower_bound(color);
            if (it != _counts.end() && it->first == color) {
                return it->second;
            }
            return 0;
        }

        void setCount(const ColorSequence& colorSequence, MarkingCount_t count) {
            setCount(colorSequence.encodedValue, count);
        }

        void setCount(const Color_t& color, MarkingCount_t count) {
            const auto it = lower_bound(color);
            _cardinality += static_cast<sMarkingCount_t>(count);

            if (it != _counts.end() && it->first == color) {
                _cardinality -= it->second;
                it->second = static_cast<sMarkingCount_t>(count);
                return;
            }
            _counts.insert(it, {color, count});
        }

        void addCount(const ColorSequence& colorSequence, sMarkingCount_t count) {
            const Color_t& color = colorSequence.encodedValue;
            const auto it = lower_bound(color);
            if (count > 0 && _cardinality > std::numeric_limits<sMarkingCount_t>::max() - count) {
                throw explicit_error{too_many_tokens};
            }
            _cardinality += count;
            if (it != _counts.end() && it->first == color) {
                it->second += count;
                return;
            }
            _counts.insert(it, {color, count});
        }

        [[nodiscard]] MarkingCount_t totalCount() const {
            return _cardinality;
        }

        SequenceMultiSet& operator+=(const SequenceMultiSet& other) {
            auto aIt = _counts.begin();
            auto bIt = other._counts.begin();
            while (aIt != _counts.end() && bIt != other._counts.end()) {
                if (aIt->first == bIt->first) {
                    if (bIt->second > 0 && _cardinality > std::numeric_limits<sMarkingCount_t>::max() - bIt->second) {
                        throw explicit_error{too_many_tokens};
                    }
                    aIt->second += bIt->second;
                    _cardinality += bIt->second;
                    ++aIt;
                    ++bIt;
                } else if (aIt->first < bIt->first) {
                    ++aIt;
                } else {
                    aIt = _counts.insert(aIt, *bIt);
                    _cardinality += bIt->second;
                    ++bIt;
                }
            }
            for (; bIt != other._counts.end(); ++bIt) {
                _counts.push_back(*bIt);
                _cardinality += bIt->second;
            }
            return *this;
        }

        SequenceMultiSet& operator-=(const SequenceMultiSet& other) {
            auto aIt = _counts.begin();
            auto bIt = other._counts.begin();
            while (aIt != _counts.end() && bIt != other._counts.end()) {
                if (aIt->first == bIt->first) {
                    aIt->second -= bIt->second;
                    _cardinality -= bIt->second;
                    ++aIt;
                    ++bIt;
                } else if (aIt->first < bIt->first) {
                    ++aIt;
                } else {
                    aIt = _counts.insert(aIt, {bIt->first, -bIt->second});
                    _cardinality -= bIt->second;
                    ++bIt;
                }
            }
            for (; bIt != other._counts.end(); ++bIt) {
                _counts.push_back(*bIt);
                _cardinality -= bIt->second;
            }
            return *this;
        }

        SequenceMultiSet& operator*=(const MarkingCount_t scalar) {
            for (auto& [color, count] : _counts) {
                count *= static_cast<sMarkingCount_t>(scalar);
            }
            if (_cardinality > std::numeric_limits<sMarkingCount_t>::max() / scalar) {
                throw explicit_error{too_many_tokens};
            }

            _cardinality *= static_cast<sMarkingCount_t>(scalar);
            return *this;
        }

        bool operator==(const SequenceMultiSet& other) const {
            if (_cardinality != other._cardinality) {
                return false;
            }
            auto aIt = _counts.begin();
            auto bIt = other._counts.begin();
            while (aIt != _counts.end() && bIt != other._counts.end()) {
                if (aIt->second == 0) {
                    ++aIt;
                    continue;
                }
                if (bIt->second == 0) {
                    ++bIt;
                    continue;
                }
                if (aIt->first != bIt->first || aIt->second != bIt->second) {
                    return false;
                }
                ++aIt;
                ++bIt;
            }
            return true;
        }

        bool operator>=(const SequenceMultiSet& other) const {
            if (_cardinality < other._cardinality) {
                return false;
            }
            auto aIt = _counts.begin();
            auto bIt = other._counts.begin();
            while (aIt != _counts.end() && bIt != other._counts.end()) {
                if (aIt->second <= 0) {
                    ++aIt;
                    continue;
                }
                if (bIt->second <= 0) {
                    ++bIt;
                    continue;
                }
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
            while (bIt != other._counts.end() && bIt->second == 0) {
                ++bIt;
            }
            if (bIt != other._counts.end()) {
                return false;
            }
            return true;
        }

        bool operator<=(const SequenceMultiSet& other) const {
            if (_cardinality > other._cardinality) {
                return false;
            }
            auto aIt = _counts.begin();
            auto bIt = other._counts.begin();
            while (aIt != _counts.end() && bIt != other._counts.end()) {
                if (aIt->second <= 0) {
                    ++aIt;
                    continue;
                }
                if (bIt->second <= 0) {
                    ++bIt;
                    continue;
                }
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
            while (aIt != _counts.end() && aIt->second == 0) {
                ++aIt;
            }
            if (aIt != _counts.end()) {
                return false;
            }
            return true;
        }

        [[nodiscard]] const std::vector<std::pair<Color_t, sMarkingCount_t>>& counts() const {
            return _counts;
        }

        void shrink() {
            auto keepIt = _counts.begin();
            for (auto cursorIt = _counts.begin(); cursorIt != _counts.end(); ++cursorIt) {
                if (cursorIt->second > 0) {
                    if (keepIt != cursorIt) {
                        keepIt->first = std::move(cursorIt->first);
                        keepIt->second = cursorIt->second;
                    }
                    ++keepIt;
                }
            }
            _counts.erase(_counts.begin() + std::distance(_counts.begin(), keepIt), _counts.end());
            _counts.shrink_to_fit();
        }

        void fixNegative() {
            for (auto& [key, count] : _counts) {
                if (count < 0) {
                    _cardinality += -count;
                    count = 0;
                }
            }
        }

        [[nodiscard]] uint32_t getHighestCount() const {
            int max = 0;
            for (auto& [key, count] : _counts) {
                max = std::max(count, max);
            }
            return max;
        }

        friend std::ostream& operator<<(std::ostream& out, const SequenceMultiSet& sequence) {
            for (const auto& [color, count] : sequence._counts) {
                if (count > 0) {
                    out << count << "'" << color << " + ";
                }
            }
            return out;
        }
    private:
        std::vector<std::pair<Color_t, sMarkingCount_t>>::iterator lower_bound(const Color_t& key) {
            return std::lower_bound(
                 _counts.begin(),
                 _counts.end(),
                 key,
                 [](const auto& a, const auto& b) {
                     return a.first < b;
                 }
            );
        }

        [[nodiscard]] std::vector<std::pair<Color_t, sMarkingCount_t>>::const_iterator clower_bound(const Color_t& key) const {
            return std::lower_bound(
                 _counts.cbegin(),
                 _counts.cend(),
                 key,
                 [](const auto& a, const auto& b) {
                     return a.first < b;
                 }
            );
        }
        std::vector<std::pair<Color_t, sMarkingCount_t>> _counts;
        sMarkingCount_t _cardinality = 0;
    };

    typedef SequenceMultiSet CPNMultiSet;
}


#endif //SEQUENCEMULTISET_H
