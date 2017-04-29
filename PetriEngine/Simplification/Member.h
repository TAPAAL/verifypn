#ifndef MEMBER_H
#define MEMBER_H
#include <algorithm>
#include <functional>
#include <cassert>
#include <cstring>
//#include "../PQL/PQL.h"

namespace PetriEngine {
    namespace Simplification {
        enum Trivial { False=0, True=1, Indeterminate=2 };
        enum MemberType { Constant, Input, Output, Regular };
        class Member {
            std::vector<int> _variables;
            int _constant = 0;
            bool _canAnalyze = false;

        public:

            Member(std::vector<int>&& vec, int constant, bool canAnalyze = true) 
                    : _variables(vec), _constant(constant), _canAnalyze(canAnalyze) {

            }
            Member(int constant, bool canAnalyse = true) 
                    : _constant(constant), _canAnalyze(canAnalyse) {
                _canAnalyze = true;
            }
            Member(){}

            virtual ~Member(){}
            
            int constant() const   { return _constant; };
            bool canAnalyze() const { return _canAnalyze; };
            size_t size() const     { return _variables.size(); }
            std::vector<int>& variables() { return _variables; }
            
            Member operator+(const Member& m) const { 
                return Member(  addVariables(*this, m),
                                _constant + m._constant,
                                _canAnalyze&&m._canAnalyze);
            }
            Member operator-(const Member& m) const { 
                return Member(  subtractVariables(*this, m), 
                                _constant - m._constant,
                                _canAnalyze&&m._canAnalyze);
            }
            Member operator*(const Member& m) const { 
                if(!isConstant()&&!m.isConstant()){
                    return Member(0, false);
                } else{
                    return Member(  multiply(*this, m), 
                                    _constant * m._constant,
                                    _canAnalyze&&m._canAnalyze);
                }
            }
            Member operator-() const { 
                Member negative(-1);
                return *this * negative;
            }

            bool isConstant() const {
                for(const int& v : _variables){
                    if(v != 0) return false;
                }
                return true;
            }
            
            MemberType getType() const {
                bool isConstant=true;
                bool isInput=true;
                bool isOutput=true;
                for(const int& v : _variables){
                    if(v < 0){
                        isConstant=false;
                        isOutput=false;
                    } else if(v > 0){
                        isConstant=false;
                        isInput=false;
                    }
                }
                if(isConstant) return MemberType::Constant;
                else if(isInput) return MemberType::Input;
                else if(isOutput) return MemberType::Output;
                else return MemberType::Regular;
            }
            
            bool operator==(const Member& m) const {
                size_t min = std::min(_variables.size(), m.size());
                size_t max = std::max(_variables.size(), m.size());
                if(memcmp(_variables.data(), m._variables.data(), sizeof(int)*min) != 0)
                {
                    return false;
                }
                for (uint32_t i = min; i < max; i++) {
                    if (i + 1 > _variables.size()) {
                        if(m._variables[i] != 0) return false;
                    } else if (i + 1 > m._variables.size()) {
                        if(_variables[i] != 0) return false;
                    } else {
                        assert(false);
                        if(_variables[i] - m._variables[i] != 0) return false;
                    }
                }
                return true;
            }

            Trivial operator<(const Member& m) const {
                return trivialLessThan(m, std::less<int>());
            }
            Trivial operator<=(const Member& m) const {
                return trivialLessThan(m, std::less_equal<int>());
            }
            Trivial operator>(const Member& m) const {
                return trivialLessThan(m, std::greater<int>());
            }
            Trivial operator>=(const Member& m) const {
                return trivialLessThan(m, std::greater_equal<int>());
            }
            
        private:
            std::vector<int> addVariables(const Member& m1, const Member& m2) const {
                uint32_t size = std::max(m1._variables.size(), m2._variables.size());
                std::vector<int> res(size);
                
                for (uint32_t i = 0; i < size; i++) {
                    if (i + 1 > m1._variables.size()) {
                        res[i] = m2._variables[i];
                    } else if (i + 1 > m2._variables.size()) {
                        res[i] = m1._variables[i];
                    } else {
                        res[i] = m1._variables[i] + m2._variables[i];
                    }
                }
                
                return res;
            }

            std::vector<int> subtractVariables(const Member& m1, const Member& m2) const {
                uint32_t size = std::max(m1._variables.size(), m2._variables.size());
                std::vector<int> res(size);
                
                for (uint32_t i = 0; i < size; i++) {
                    if (i + 1 > m1._variables.size()) {
                        res[i] =  0 - m2._variables[i];
                    } else if (i + 1 > m2._variables.size()) {
                        res[i] = m1._variables[i];
                    } else {
                        res[i] = m1._variables[i] - m2._variables[i];
                    }
                }                
                return res;
            }

            std::vector<int> multiply(const Member& m1, const Member& m2) const {

                if (m1.isConstant() != m2.isConstant()){
                    if (!m1.isConstant()){
                        std::vector<int> res = m1._variables;
                        for(auto& v : res) v *= m2._constant;
                        return res;
                    } else if (!m2.isConstant()){
                        std::vector<int> res = m2._variables;
                        for(auto& v : res) v *= m1._constant;
                        return res;
                    }
                }
                return {};
            }
            
            Trivial trivialLessThan(const Member& m2, std::function<bool (int, int)> compare) const {
                MemberType type1 = getType();
                MemberType type2 = m2.getType();
                
                // self comparison
                if (*this == m2)
                    return compare(_constant, m2._constant) ? Trivial::True : Trivial::False;
                    
                // constant < constant/input/output
                if (type1 == MemberType::Constant){
                    if(type2 == MemberType::Constant){
                        return compare(_constant, m2._constant) ? Trivial::True : Trivial::False;
                    }
                    else if(type2 == MemberType::Input && !compare(_constant, m2._constant)){
                        return Trivial::False;
                    } 
                    else if(type2 == MemberType::Output && compare(_constant, m2._constant)){
                        return Trivial::True;
                    } 
                } 
                // input < output/constant
                else if (type1 == MemberType::Input 
                        && (type2 == MemberType::Constant || type2 == MemberType::Output) 
                        && compare(_constant, m2._constant)) {
                    return Trivial::True;
                } 
                // output < input/constant
                else if (type1 == MemberType::Output 
                        && (type2 == MemberType::Constant || type2 == MemberType::Input) 
                        && !compare(_constant, m2._constant)) {
                    return Trivial::False;
                } 
                return Trivial::Indeterminate;
            }

        };
    }
}

#endif /* MEMBER_H */
