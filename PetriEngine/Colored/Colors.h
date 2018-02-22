/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Colors.h
 * Author: andreas
 *
 * Created on February 19, 2018, 8:22 PM
 */

#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <string.h>
#include <vector>

namespace PetriEngine {
    namespace Colored {
        class ColorType;
        
        // Should make constructor protected, and make ColorType Friendly
        class Color {
        private:
            const Color** _tuple;
            const size_t _tupleSize;
            ColorType* _colorType;
            const char* _colorName;
            uint32_t _id;
            
        public:
            Color(ColorType* colorTypeName, uint32_t id, const Color** colors, const size_t colorSize);
            Color(ColorType* colorTypeName, uint32_t id, const char* color);
            
            bool isTuple() const {
                return _tupleSize > 1;
            }
            
            std::string getColorName() const {
                if (this->isTuple()) {
                    throw "Cannot get color from a tuple color.";
                }
                return std::string(_colorName);
            }
            
            const Color* operator[] (size_t index) const;
            bool operator< (const Color& other) const;
            
            bool operator== (const Color& other) const {
                return _colorType == other._colorType && _id == other._id;
            }
            
            const Color& operator++ () const;
            const Color& operator-- () const;
            
            std::string toString() const;
        };
        
        /*
         *  Singleton pattern from: 
         * https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
         */
        class DotConstant : public Color {
        private:
            DotConstant();
            
        public:
            static const DotConstant* dotConstant() {
                static DotConstant _instance;
                
                return &_instance;
            }
            
            DotConstant(DotConstant const&) = delete;
            void operator=(DotConstant const&) = delete;
        };
        
        class ColorType {
        private:
            std::vector<Color> _colors;
            
        public:
            size_t size() const {
                return _colors.size();
            }
            
            const Color& operator[] (size_t index) const {
                return _colors[index];
            }
            
            const Color& operator[] (int index) const {
                return _colors[index];
            }
            
            const Color& operator[] (uint32_t index) const {
                return _colors[index];
            }
            
            const Color& operator[] (const char* index) const;
            
            const Color& operator[] (std::string index) const {
                return (*this)[index.c_str()];
            }
            
            bool operator== (const ColorType& other) const {
                return true; // TODO
            }
        };
        
        struct Variable {
            char* name;
            ColorType* colorType;
        };
    }
}

#endif /* COLORS_H */

