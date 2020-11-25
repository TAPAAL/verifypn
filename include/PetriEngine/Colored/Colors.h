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
#include <iostream>
#include <cassert>

//#include "ColoredNetStructures.h"
#include "../TAR/range.h"

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

            void getColorConstraints(Reachability::interval_t *constraintsVector, uint32_t *index) const;
            
            std::vector<const Color*> getTupleColors() const {
                return _tuple;
            }

            void getTupleId(std::vector<uint32_t> *idVector) const;

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

            virtual size_t productSize() {
                return 1;
            }

            virtual std::vector<size_t> getConstituentsSizes(){
                std::vector<size_t> result;                
                result.push_back(_colors.size());
                
                return result;
            }

            virtual Reachability::interval_t getFullInterval(){
                Reachability::interval_t interval;
                interval.addRange(Reachability::range_t(0, size()-1));
                return interval;
            }

            virtual void getColortypes(std::vector<ColorType *> &colorTypes){
                colorTypes.push_back(this);
            }
            
            virtual const Color& operator[] (size_t index) {
                return _colors[index];
            }
            
            virtual const Color& operator[] (int index) {
                return _colors[index];
            }
            
            virtual const Color& operator[] (uint32_t index) {
                assert(index < _colors.size());
                return _colors[index];
            }
            
            virtual const Color* operator[] (const char* index);
            
            virtual const Color* operator[] (const std::string& index) {
                return (*this)[index.c_str()];
            }

            virtual const Color* getColor(std::vector<uint32_t> ids){
                assert(ids.size() == 1);
                return &_colors[ids[0]];
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

            virtual size_t productSize() {
                size_t size = 0;
                for (auto ct : constituents){
                    size += ct->productSize();
                }
                return size;
            }

            std::vector<size_t> getConstituentsSizes() override{
                std::vector<size_t> result;
                for (auto ct : constituents) {
                    result.push_back(ct->size());
                }
                return result;
            }

            Reachability::interval_t getFullInterval() override{
                Reachability::interval_t interval;
                for(auto ct : constituents) {
                    interval.addRange(Reachability::range_t(0, ct->size()-1));
                }                
                return interval;
            }

            void getColortypes(std::vector<ColorType *> &colorTypes) override{
                for(auto ct : constituents){
                    ct->getColortypes(colorTypes);
                }
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

            const ColorType* getNestedColorType(size_t index) {
                return constituents[index];
            }

            const Color* getColor(std::vector<uint32_t> ids){
                assert(ids.size() == constituents.size());
                std::vector<const Color *> colors;
                for(uint32_t i = 0; i < ids.size(); i++){
                    colors.push_back(&constituents[i]->operator[](i));
                }
                return getColor(colors);
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

        struct ColorFixpoint {
            Reachability::intervalTuple_t constraints;
            bool inQueue;
            uint32_t productSize;

            bool constainsColor(std::pair<const PetriEngine::Colored::Color *const, std::vector<uint32_t>> constPair) {
                std::unordered_map<uint32_t, bool> contained;
                for(auto interval : constraints._intervals) {
                    for(uint32_t id : constPair.second){
                        
                        if(contained[id] != true){
                            contained[id] = interval[id].contains(constPair.first->getId());
                        }                        
                    }
                }

                for(auto pair : contained){
                    if (!pair.second){
                        return false;
                    }
                }
                return true;
            }
        };

        struct ArcIntervals {
            std::unordered_map<Colored::Variable *, std::pair<uint32_t, int32_t>> _varIndexModMap;
            Reachability::intervalTuple_t _intervalTuple;
            Colored::ColorFixpoint * _source;
            //are these arc intervals or something seperate
            //std::unordered_map<Colored::ColorFixpoint *, std::pair<std::set<uint32_t>, int32_t>> _parentPlaceIndexMap;
            

            ArcIntervals() {
            }

            ArcIntervals(Colored::ColorFixpoint * source) : _source(source){
            }

            ArcIntervals(Colored::ColorFixpoint * source, std::unordered_map<Colored::Variable *, std::pair<uint32_t, int32_t>> varIndexModMap) : _source(source), _varIndexModMap(varIndexModMap) {
            };

            ArcIntervals(Colored::ColorFixpoint * source, std::unordered_map<Colored::Variable *, std::pair<uint32_t, int32_t>> varIndexModMap,  Reachability::intervalTuple_t ranges) : _source(source), _varIndexModMap(varIndexModMap), _intervalTuple(ranges) {
            };

            size_t size() {
                return _intervalTuple.size();
            }

            Reachability::interval_t& operator[] (size_t index) {
                return _intervalTuple[index];
            }
            
            Reachability::interval_t& operator[] (int index) {
                return _intervalTuple[index];
            }
            
            Reachability::interval_t& operator[] (uint32_t index) {
                assert(index < _intervalTuple.size());
                return _intervalTuple[index];
            }

            Reachability::interval_t& back(){
                return _intervalTuple.back();
            }

            bool hasValidIntervals(){
                return _intervalTuple.hasValidIntervals();
            }

            bool containsVariable(Colored::Variable * var){
                for (auto varModPair : _varIndexModMap){
                    if(varModPair.first == var){
                        return true;
                    }
                }
                return false;
            }

            std::set<Colored::Variable *> getVariables(){
                std::set<Colored::Variable *> res;
                for (auto varModPair : _varIndexModMap){
                    res.insert(varModPair.first);
                }
                return res;
            }

            void resetIntervals(){
                _intervalTuple = _source->constraints;
            }

            uint32_t getVarPosition(Colored::Variable * var){

                for (auto varIdModPair : _varIndexModMap){
                    if(varIdModPair.first == var){
                        return varIdModPair.second.first;
                    }
                }
            }

            void print() {
                std::cout << "[ ";
                for(auto varModifierPair : _varIndexModMap){
                    std::cout << "(" << varModifierPair.first->name << ", " << varModifierPair.first->colorType->productSize() << ", " << varModifierPair.second.second <<  ") ";
                }
                std::cout << "]" << std::endl;
                _intervalTuple.print();
            }
        };
        
    }
}

#endif /* COLORS_H */

