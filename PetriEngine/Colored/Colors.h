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
        public:
            friend std::ostream& operator<< (std::ostream& stream, const Color& color);
            
        protected:
            const std::vector<const Color*> _tuple;
            ColorType* _colorType;
            std::string _colorName;
            uint32_t _id;
            
        public:
            Color(ColorType* colorType, uint32_t id, std::vector<const Color*>& colors);
            Color(ColorType* colorType, uint32_t id, const char* color);
            
            bool isTuple() const {
                return _tuple.size() > 1;
            }
            
            const std::string& getColorName() const {
                if (this->isTuple()) {
                    throw "Cannot get color from a tuple color.";
                }
                return _colorName;
            }
            
            ColorType* getColorType() const {
                return _colorType;
            }
            
            uint32_t getId() const {
                return _id;
            }
            
            const Color* operator[] (size_t index) const;
            bool operator< (const Color& other) const;
            bool operator> (const Color& other) const;
            bool operator<= (const Color& other) const;
            bool operator>= (const Color& other) const;
            
            bool operator== (const Color& other) const {
                return _colorType == other._colorType && _id == other._id;
            }
            bool operator!= (const Color& other) const {
                return !((*this) == other);
            }
            
            const Color& operator++ () const;
            const Color& operator-- () const;
            
            std::string toString() const;
            static std::string toString(const Color* color);
            static std::string toString(const std::vector<const Color*>& colors);
        };
        
        /*
         *  Singleton pattern from: 
         * https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
         */
        class DotConstant : public Color {
        private:
            DotConstant();
            
        public:
            static const Color* dotConstant() {
                static DotConstant _instance;
                
                return &_instance;
            }
            
            DotConstant(DotConstant const&) = delete;
            void operator=(DotConstant const&) = delete;

            bool operator== (const DotConstant& other) {
                return true;
            }
        };
        
        class ColorType {
        public:
            typedef std::vector<Color>::iterator iterator;
            typedef std::vector<Color>::const_iterator const_iterator;
            
        private:
            std::vector<Color> _colors;
            uintptr_t _id;
            
        public:
            ColorType(std::vector<ColorType*> elements);
            ColorType() : _colors() {
                _id = (uintptr_t)this;
            }
            
            void addColor(const char* colorName);
            void addColor(std::vector<const Color*>& colors);
            void addDot() {
                _colors.push_back(*DotConstant::dotConstant());
            }
            
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
                return _id == other._id;
            }

            uintptr_t getId() {
                return _id;
            }
            
            iterator begin() {
                return _colors.begin();
            }
            
            const_iterator begin() const {
                return _colors.begin();
            }
            
            iterator end() {
                return _colors.end();
            }
            
            const_iterator end() const {
                return _colors.end();
            }
        };
        
        struct Variable {
            std::string name;
            ColorType* colorType;
        };
        
        struct Binding {
            Variable* var;
            const Color* color;
            
            bool operator==(Binding& other) {
                return var->name.compare(other.var->name);
            }
        };
    }
}

#endif /* COLORS_H */

