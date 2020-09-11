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
            }
            Member(){}

            virtual ~Member(){}
            
            int constant() const    { return _constant; };
            bool canAnalyze() const { return _canAnalyze; };
            size_t size() const     { return _variables.size(); }
            std::vector<int>& variables() { return _variables; }
            const std::vector<int>& variables() const { return _variables; }
            
            Member& operator+=(const Member& m) { 
                auto tc = _constant + m._constant;
                auto ca = _canAnalyze && m._canAnalyze;                
                addVariables(m);
                _constant = tc;
                _canAnalyze = ca;
                return *this;
            }
            
            Member& operator-=(const Member& m) { 
                auto tc = _constant - m._constant;
                auto ca = _canAnalyze && m._canAnalyze;                
                subtractVariables(m);
                _constant = tc;
                _canAnalyze = ca;
                return *this;
            }
            
            Member& operator*=(const Member& m) { 
                if(!isZero() && !m.isZero()){
                    _canAnalyze = false;
                    _constant = 0;
                    _variables.clear();
                } else{
                    auto tc = _constant * m._constant;
                    auto ca = _canAnalyze && m._canAnalyze;
                    multiply(m);
                    _constant = tc;
                    _canAnalyze = ca;
                }
                return *this;
            }            

            bool substrationIsZero(const Member& m2) const
            {
                uint32_t min = std::min(_variables.size(), m2._variables.size());
                uint32_t i = 0;
                for(; i < min; i++) {
                    if(_variables[i] != m2._variables[i]) return false;
                }
                
                for(; i < _variables.size(); ++i)
                {
                    if(_variables[i] != 0) return false;
                }

                for(; i < m2._variables.size(); ++i)
                {
                    if(m2._variables[i] != 0) return false;
                }
                return true;
            }
            
            bool isZero() const {
                for(const int& v : _variables){
                    if(v != 0) return false;
                }
                return true;
            }
            
            MemberType getType() const {
                bool isConstant = true;
                bool isInput = true;
                bool isOutput = true;
                for(const int& v : _variables){
                    if(v < 0){
                        isConstant = false;
                        isOutput = false;
                    } else if(v > 0){
                        isConstant = false;
                        isInput = false;
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
                    if (i >= _variables.size()) {
                        if(m._variables[i] != 0) return false;
                    } else if (i >= m._variables.size()) {
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
                return m.trivialLessThan(*this, std::less<int>());
            }
            Trivial operator>=(const Member& m) const {
                return m.trivialLessThan(*this, std::less_equal<int>());
            }
            
        private:
            void addVariables(const Member& m2) {
                uint32_t size = std::max(_variables.size(), m2._variables.size());
                _variables.resize(size, 0);

                for (uint32_t i = 0; i < size; i++) {
                    if (i >= m2._variables.size()) {
                        break;
                    } else {
                        _variables[i] += m2._variables[i];
                    }
                }
            }

            void subtractVariables(const Member& m2) {
                uint32_t size = std::max(_variables.size(), m2._variables.size());
                _variables.resize(size, 0);
                
                for (uint32_t i = 0; i < size; i++) {
                    if (i >= m2._variables.size()) {
                        break;
                    } else {
                        _variables[i] -= m2._variables[i];
                    }
                }           
            }

            void multiply(const Member& m2) {

                if (isZero() != m2.isZero()){
                    if (!isZero()){
                        for(auto& v : _variables) v *= m2._constant;
                        return;
                    } else if (!m2.isZero()){
                        _variables = m2._variables;
                        for(auto& v : _variables) v *= _constant;
                        return;
                    }
                }
                _variables.clear();
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
