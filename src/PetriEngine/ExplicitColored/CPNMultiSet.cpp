#include "PetriEngine/ExplicitColored/CPNMultiSet.h"
#include "utils/errors.h"
namespace PetriEngine {
    namespace ExplicitColored {
        MarkingCount_t CPNMultiSet::getCount(const std::vector<Color_t>& color) const {
            for (const auto& colorCount : _counts) {
                if (color.size() != colorCount.first.size())
                    throw base_error("Tried to access multiset with mismatched types");
                
                bool matches = true;
                for (size_t i = 0; i < color.size(); i++) {
                    if (color[i] != colorCount.first[i]) {
                        matches = false;
                        break;
                    }
                }

                if (!matches)
                    continue;

                return colorCount.second;
            }
            return 0;
        }
        
        void CPNMultiSet::setCount(std::vector<Color_t> color, MarkingCount_t count) {
            for (auto& colorCount : _counts) {
                bool found = true;

                if (color.size() != colorCount.first.size())
                    throw base_error("Tried to access multiset with mismatched types");

                for (size_t i = 0; i < color.size(); i++) {
                    if (color[i] != colorCount.first[i]) {
                        found = false;
                        break;
                    }
                }

                if (!found) {
                    continue;
                }
                _cardinality = (_cardinality + count) - colorCount.second;
                
                colorCount.second = count;
                return;
            }
            _cardinality += count;
            _counts.emplace_back(std::make_pair(std::move(color), count));
        }

        CPNMultiSet& CPNMultiSet::operator+=(const CPNMultiSet& other) {
            for (auto& b : other._counts) {
                bool foundMatch = false;
                for (auto& a : _counts) {
                    foundMatch = true;
                    if (a.first.size() != b.first.size())
                        throw base_error("Tried to access multiset with mismatched types");

                    for (size_t i = 0; i < a.first.size(); i++) {
                        if (a.first[i] != b.first[i]) {
                            foundMatch = false;
                            break;
                        }
                    }

                    if (!foundMatch) {
                        continue;
                    }

                    a.second += b.second;
                }
                if (!foundMatch) {
                    _counts.push_back(b);
                }
            }
            _cardinality += other._cardinality;
            return *this;
        }

        CPNMultiSet& CPNMultiSet::operator-=(const CPNMultiSet& other) {
             for (auto& b : other._counts) {
                for (auto& a : _counts) {
                    if (a.first.size() != b.first.size())
                        throw base_error("Tried to access multiset with mismatched types");

                    bool found = true;
                    for (size_t i = 0; i < a.first.size(); i++) {
                        if (a.first[i] != b.first[i]) {
                            found = false;
                            break;
                        }
                    }
                    if (!found) {
                        continue;
                    }

                    if (b.second > a.second) {
                        _cardinality -= a.second;
                        a.second = 0;
                    } else {
                        _cardinality = (_cardinality + b.second) - a.second;
                        a.second -= b.second;
                    }
                    
                    a.second -= b.second;
                    break;
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
    }
}