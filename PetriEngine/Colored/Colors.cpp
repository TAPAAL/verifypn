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
#include <cassert>

namespace PetriEngine {
    namespace Colored {
        std::ostream& operator<<(std::ostream& stream, const Color& color) {
            stream << color.toString();
            return stream;
        }
        
        Color::Color(ColorType* colorType, uint32_t id, std::vector<const Color*>& colors)
                : _tuple(colors), _colorType(colorType), _colorName(""), _id(id)
        {
            if (colorType != nullptr)
                assert(id <= colorType->size());
        }
        
        Color::Color(ColorType* colorType, uint32_t id, const char* color)
                : _tuple(), _colorType(colorType), _colorName(color), _id(id)
        {
            if (colorType != nullptr)
                assert(id <= colorType->size());
        }
        
        
        const Color* Color::operator[] (size_t index) const {
            if (!this->isTuple()) {
                throw "Cannot access tuple if not a tuple color";
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
            assert(_id + 1 < _colorType->size());
            return (*_colorType)[_id + 1];
        }
        
        const Color& Color::operator-- () const {
            //printf("Predecessor of %s\n", toString().c_str());
            if (_id <= 0) {
                return (*_colorType)[_colorType->size() - 1];
            }
            assert(_id - 1 >= 0);
            return (*_colorType)[_id - 1];
        }
        
        std::string Color::toString() const {
            return toString(this);
        }
        
        std::string Color::toString(const Color* color) {
            if (color->isTuple()) {
                std::ostringstream oss;
                oss << "(";
                for (size_t i = 0; i < color->_tuple.size(); i++) {
                    oss << color->_tuple[i]->toString();
                    if (i < color->_tuple.size() - 1) oss << ",";
                }
                oss << ")";
                return oss.str();
            }
            //printf("%s\n", color->_colorName);
            return std::string(color->_colorName);
        }
        
        std::string Color::toString(const std::vector<const Color*>& colors) {
            std::ostringstream oss;
            if (colors.size() > 1)
                oss << "(";
            
            for (size_t i = 0; i < colors.size(); i++) {
                oss << colors[i]->toString();
                if (i < colors.size() - 1) oss << ",";
            }

            if (colors.size() > 1)
                oss << ")";
            return oss.str();
        }
        
        
        DotConstant::DotConstant() : Color(0, 0, "dot")
        {
        }
        
        
        void ColorType::addColor(const char* colorName) {
            _colors.push_back(Color(this, _colors.size(), colorName));
        }
        
        void ColorType::addColor(std::vector<const Color*>& colors) {
            _colors.push_back(Color(this, (uint32_t)_colors.size(), colors));
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