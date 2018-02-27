/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Colors.h"
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <iterator>

namespace PetriEngine {
    namespace Colored {
        std::ostream& operator<<(std::ostream& stream, const Color& color) {
            stream << color.toString();
            return stream;
        }
        
        Color::Color(ColorType* colorType, uint32_t id, const Color** colors, const size_t colorSize)
                : _tuple(colors), _tupleSize(colorSize), _colorType(colorType), _colorName(0), _id(id)
        {
        }
        
        Color::Color(ColorType* colorType, uint32_t id, const char* color)
                : _tuple(0), _tupleSize(1), _colorType(colorType), _colorName(color), _id(id)
        {
        }
        
        
        const Color* Color::operator[] (size_t index) const {
            if (!this->isTuple()) {
                throw "Cannot access tuple if not a tuple color";
            }
            if (index >= _tupleSize) {
                throw "Index out of range";
            }
            return _tuple[index];
        }
        
        bool Color::operator< (const Color& other) const {
            if (_colorType == other._colorType) {
                throw "Cannot compare colors from different types";
            }
            return _id < other._id;
        }
        
        bool Color::operator> (const Color& other) const {
            if (_colorType == other._colorType) {
                throw "Cannot compare colors from different types";
            }
            return _id > other._id;
        }
        
        bool Color::operator<= (const Color& other) const {
            if (_colorType == other._colorType) {
                throw "Cannot compare colors from different types";
            }
            return _id <= other._id;
        }
        
        bool Color::operator>= (const Color& other) const {
            if (_colorType == other._colorType) {
                throw "Cannot compare colors from different types";
            }
            return _id >= other._id;
        }
        
        const Color& Color::operator++ () const {
            if (_id >= _colorType->size() - 1) {
                return (*_colorType)[0];
            }
            return (*_colorType)[_id + 1];
        }
        
        const Color& Color::operator-- () const {
            if (_id <= 0) {
                return (*_colorType)[_colorType->size() - 1];
            }
            return (*_colorType)[_id - 1];
        }
        
        std::string Color::toString() const {
            return toString(this);
        }
        
        std::string Color::toString(const Color* color) {
            if (color->isTuple()) {
                std::ostringstream oss;
                oss << "(";
                for (size_t i = 0; i < color->_tupleSize; i++) {
                    oss << color->_tuple[i]->toString();
                    if (i < color->_tupleSize - 1) oss << ",";
                }
                oss << ")";
                return oss.str();
            }
            return std::string(color->_colorName);
        }
        
        std::string Color::toString(const std::vector<const Color*>& colors) {
            std::ostringstream oss;
            oss << "(";
            std::copy(colors.begin(), colors.end(), std::ostream_iterator<const Color*>(oss, ","));
            oss << ")";
            return oss.str();
        }
        
        
        DotConstant::DotConstant() : Color(0, 0, "dot")
        {
        }
        
        
        void ColorType::addColor(const char* colorName) {
            _colors.push_back(Color(this, _colors.size(), colorName));
        }
        
        const Color& ColorType::operator[] (const char* index) const {
            for (size_t i = 0; i < _colors.size(); i++) {
                if (strcmp(_colors[i].toString().c_str(), index) == 0)
                    return _colors[i];
            }
            throw "Index out of bounds";
        }
        
    }
}