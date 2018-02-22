/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Colors.h"
#include <sstream>
#include <string>

namespace PetriEngine {
    namespace Colored {
        Color::Color(ColorType* colorTypeName, uint32_t id, const Color** colors, const size_t colorSize)
            : _tuple(colors), _tupleSize(colorSize), _colorType(colorTypeName), _id(id), _colorName(0)
        {
        }
        
        Color::Color(ColorType* colorTypeName, uint32_t id, const char* color)
                : _tuple(0), _tupleSize(1), _colorType(colorTypeName), _id(id), _colorName(color)
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
            if (this->isTuple()) {
                std::ostringstream oss;
                oss << "(";
                for (size_t i = 0; i < _tupleSize; i++) {
                    oss << _tuple[i]->toString();
                    if (i < _tupleSize - 1) oss << ",";
                }
                oss << ")";
                return oss.str();
            }
            return std::string(_colorName);
        }
        
        
        DotConstant::DotConstant() : Color(0, 0, "dot")
        {
        }
        
        
        const Color& ColorType::operator[] (const char* index) const {
            for (size_t i = 0; i < _colors.size(); i++) {
                if (!strcmp(_colors[i].toString().c_str(), index))
                    return _colors[i];
            }
            throw "Index out of bounds";
        }
        
    }
}