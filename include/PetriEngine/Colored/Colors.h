/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include "Intervals.h"
#include "utils/errors.h"

namespace PetriEngine {
    namespace Colored {
        class ColorType;
        class Variable;
        class Color;

        typedef std::unordered_map<std::string, const ColorType*> ColorTypeMap;
        typedef std::unordered_map<const Variable *, const Color *> BindingMap;

        class Color final {
        public:
            friend std::ostream& operator<< (std::ostream& stream, const Color& color);

        protected:
            const std::vector<const Color*> _tuple;
            const ColorType * const _colorType;
            std::string _colorName;
            uint32_t _id;
            std::string _displayName;

        public:
            Color(const ColorType* colorType, uint32_t id, std::vector<const Color*>& colors);
            Color(const ColorType* colorType, uint32_t id, const char* color);
            Color(const ColorType* colorType, uint32_t id, const char* color, const char* colorname);
            ~Color() {}

            bool isTuple() const {
                return _tuple.size() > 1;
            }

            void getColorConstraints(Colored::interval_t& constraintsVector, uint32_t& index) const;

            const std::vector<const Color*>& getTupleColors() const {
                return _tuple;
            }

            void getTupleId(std::vector<uint32_t>& idVector) const;

            const std::string& getColorName() const {
                if (this->isTuple()) {
                    throw base_error("Cannot get color from a tuple color.");
                }
                return _colorName;
            }

            const std::string& getDisplayName() const {
                if (this->isTuple()) {
                    throw base_error("Cannot get display name from a tuple color.");
                }
                return _displayName;
            }

            const ColorType* getColorType() const {
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

        class ColorType {
        private:
            std::vector<Color> _colors;
            std::string _name;
        public:

            ColorType(std::string name = "Undefined") : _colors(), _name(std::move(name)) {
            }

            virtual ~ColorType() = default;

            static const ColorType* dotInstance();
            virtual void addColor(const char* colorName);
            virtual void addColor(const char* colorName, const char* colorName1);

            virtual size_t size() const {
                return _colors.size();
            }

            virtual size_t size(const std::vector<bool> &excludedFields) const {
                return _colors.size();
            }

            virtual size_t productSize() const {
                return 1;
            }

            virtual std::vector<size_t> getConstituentsSizes() const{
                std::vector<size_t> result;
                result.push_back(_colors.size());

                return result;
            }

            virtual Colored::interval_t getFullInterval() const{
                Colored::interval_t interval;
                interval.addRange(0, size()-1);
                return interval;
            }

            virtual void getColortypes(std::vector<const ColorType *> &colorTypes) const{
                colorTypes.emplace_back(this);
            }

            virtual const Color& operator[] (size_t index) const {
                return _colors[index];
            }

            virtual const Color* operator[] (const char* index) const;

            virtual const Color* operator[] (const std::string& index) const {
                return (*this)[index.c_str()];
            }

            virtual const Color* getColor(const std::vector<uint32_t> &ids) const {
                assert(ids.size() == 1);
                return &_colors[ids[0]];
            }

            const std::string& getName() const {
                return _name;
            }

            std::vector<Color>::const_iterator begin() const {
                return _colors.begin();
            }

            std::vector<Color>::const_iterator end() const {
                return _colors.end();
            }

            virtual bool isProduct() const {
                return false;
            }
        };

        class ProductType : public ColorType {
        private:
            std::vector<const ColorType*> _constituents;
            mutable std::unordered_map<size_t,Color> _cache;

        public:
            ProductType(const std::string& name = "Undefined") : ColorType(name) {}
            ~ProductType() {
                _cache.clear();
            }

            void addType(const ColorType* type) {
                _constituents.push_back(type);
            }

            void addColor(const char* colorName) override {}

            size_t size() const override {
                size_t product = 1;
                for (auto* ct : _constituents) {
                    product *= ct->size();
                }
                return product;
            }

            size_t size(const std::vector<bool> &excludedFields) const override {
                size_t product = 1;
                for (uint32_t i = 0; i < _constituents.size(); i++) {
                    if(!excludedFields[i]){
                        product *= _constituents[i]->size();
                    }
                }
                return product;
            }

            virtual size_t productSize() const{
                size_t size = 0;
                for (auto* ct : _constituents){
                    size += ct->productSize();
                }
                return size;
            }

            std::vector<size_t> getConstituentsSizes() const override{
                std::vector<size_t> result;
                for (auto* ct : _constituents) {
                    result.push_back(ct->size());
                }
                return result;
            }

            Colored::interval_t getFullInterval() const override{
                Colored::interval_t interval;
                for(auto ct : _constituents) {
                    interval.addRange(Reachability::range_t(0, ct->size()-1));
                }
                return interval;
            }

            void getColortypes(std::vector<const ColorType *> &colorTypes) const override{
                for(auto ct : _constituents){
                    ct->getColortypes(colorTypes);
                }
            }

            bool containsTypes(const std::vector<const ColorType*>& types) const {
                if (_constituents.size() != types.size()) return false;

                for (size_t i = 0; i < _constituents.size(); ++i) {
                    if (!(_constituents[i] == types[i])) {
                        return false;
                    }
                }

                return true;
            }

            const ColorType* getNestedColorType(size_t index) const {
                return _constituents[index];
            }

            const Color* getColor(const std::vector<uint32_t> &ids) const override;

            const Color* getColor(const std::vector<const Color*>& colors) const;

            const Color& operator[](size_t index) const override;
            const Color* operator[](const char* index) const override;
            const Color* operator[](const std::string& index) const override;

            virtual bool isProduct() const {
                return true;
            }
        };

        struct Variable {
            std::string name;
            const ColorType* colorType;

        };

        struct ColorFixpoint {
            Colored::interval_vector_t constraints;
            bool inQueue;
        };

        struct ColorTypePartition {
            std::vector<const Color *> colors;
            std::string name;
        };

        typedef std::unordered_map<uint32_t, const Colored::Variable *> PositionVariableMap;
        //Map from variables to a vector of maps from variable positions to the modifiers applied to the variable in that position
        typedef std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> VariableModifierMap;
        typedef std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::interval_vector_t> VariableIntervalMap;
        typedef std::unordered_map<uint32_t, std::vector<const Color*>> PositionColorsMap;
    }
}

#endif /* COLORS_H */

