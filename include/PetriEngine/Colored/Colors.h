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
#include <utility>
#include <vector>
#include <unordered_map>

namespace PetriEngine {
    namespace Colored {
        class ColorType;

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
            ~Color() {}
            
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
            class iterator {
            private:
                ColorType& type;
                size_t index;

            public:
                iterator(ColorType& type, size_t index) : type(type), index(index) {}

                const Color& operator++() {
                    return type[++index];
                }

                const Color& operator++(int) {
                    return type[index++];
                }

                const Color& operator--() {
                    return type[--index];
                }
                const Color& operator--(int) {
                    return type[index--];
                }

                const Color& operator*() {
                    return type[index];
                }

                const Color* operator->() {
                    return &type[index];
                }

                bool operator==(iterator& other) {
                    return type == other.type && index == other.index;
                }

                bool operator!=(iterator& other) {
                    return !(type == other.type) || index != other.index;
                }
            };
            
        private:
            std::vector<Color> _colors;
            uintptr_t _id;
            std::string _name;
            
        public:
            ColorType(std::vector<ColorType*> elements);
            ColorType(std::string name = "Undefined") : _colors(), _name(std::move(name)) {
                _id = (uintptr_t)this;
            }
            
            virtual void addColor(const char* colorName);
            virtual void addColor(std::vector<const Color*>& colors);
            virtual void addDot() {
                _colors.push_back(*DotConstant::dotConstant());
            }
            
            virtual size_t size() const {
                return _colors.size();
            }
            
            virtual const Color& operator[] (size_t index) {
                return _colors[index];
            }
            
            virtual const Color& operator[] (int index) {
                return _colors[index];
            }
            
            virtual const Color& operator[] (uint32_t index) {
                return _colors[index];
            }
            
            virtual const Color* operator[] (const char* index);
            
            virtual const Color* operator[] (const std::string& index) {
                return (*this)[index.c_str()];
            }
            
            bool operator== (const ColorType& other) const {
                return _id == other._id;
            }

            uintptr_t getId() {
                return _id;
            }

            const std::string& getName() const {
                return _name;
            }
            
            iterator begin() {
                return {*this, 0};
            }

            iterator end() {
                return {*this, size()};
            }
        };

        class ProductType : public ColorType {
        private:
            std::vector<ColorType*> constituents;
            std::unordered_map<size_t,Color> cache;

        public:
            ProductType(const std::string& name = "Undefined") : ColorType(name) {}
            ~ProductType() {
                cache.clear();
            }

            void addType(ColorType* type) {
                constituents.push_back(type);
            }

            void addColor(const char* colorName) override {}
            void addColor(std::vector<const Color*>& colors) override {}
            void addDot() override {}

            size_t size() const override {
                size_t product = 1;
                for (auto ct : constituents) {
                    product *= ct->size();
                }
                return product;
            }

            bool containsTypes(const std::vector<const ColorType*>& types) const {
                if (constituents.size() != types.size()) return false;

                for (size_t i = 0; i < constituents.size(); ++i) {
                    if (!(*constituents[i] == *types[i])) {
                        return false;
                    }
                }

                return true;
            }

            const Color* getColor(const std::vector<const Color*>& colors);

            const Color& operator[](size_t index) override;
            const Color& operator[](int index) override {
                return operator[]((size_t)index);
            }
            const Color& operator[](uint32_t index) override {
                return operator[]((size_t)index);
            }

            const Color* operator[](const char* index) override;
            const Color* operator[](const std::string& index) override;
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

