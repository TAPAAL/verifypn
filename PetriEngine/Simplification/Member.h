#ifndef MEMBER_H
#define MEMBER_H
#include <algorithm>
#include <functional>
//#include "../PQL/PQL.h"

namespace PetriEngine {
    namespace Simplification {
        enum Trivial { False=0, True=1, Indeterminate=2 };
        enum MemberType { Constant, Input, Output, Regular };
        class Member {
        public:
            std::vector<double> variables;
            double constant;
            bool canAnalyze;

            Member(std::vector<double> vec, double constant) : variables(vec), constant(constant) {
                canAnalyze = true;
            }
            Member(double constant) : constant(constant) {
                canAnalyze = true;
            }
            Member(){}

            virtual ~Member(){}

            Member operator+(const Member& m) { 
                Member res;
                res.constant = constant + m.constant;
                res.variables = addVariables(*this, m);
                res.canAnalyze = canAnalyze&&m.canAnalyze;
                return res;
            }
            Member operator-(const Member& m) { 
                Member res;
                res.constant = constant - m.constant;
                res.variables = subtractVariables(*this, m);
                res.canAnalyze = canAnalyze&&m.canAnalyze;
                return res;
            }
            Member operator*(const Member& m) { 
                Member res;
                if(!isConstant()&&!m.isConstant()){
                    res.canAnalyze = false;
                } else{
                    res.constant = constant * m.constant;            
                    res.canAnalyze = canAnalyze&&m.canAnalyze;
                    res.variables = multiply(*this, m);
                }
                return res;
            }
            Member operator-() { 
                Member res;
                Member negative(-1);
                res = *this * negative;
                return res;
            }

            bool isConstant() const {
                for(const double& v : variables){
                    if(v != 0) return false;
                }
                return true;
            }
            
            MemberType getType(){
                bool isConstant=true;
                bool isInput=true;
                bool isOutput=true;
                for(const double& v : variables){
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
            
            bool operator==(const Member& m) {
                return (*this-m).isConstant();
            }
            bool operator!=(const Member& m) {
                return !((*this-m).isConstant());
            }
            Trivial operator<(const Member& m) {
                return trivialLessThan(*this, m, std::less<int>());
            }
            Trivial operator<=(const Member& m) {
                return trivialLessThan(*this, m, std::less_equal<int>());
            }
            Trivial operator>(const Member& m) {
                return trivialLessThan(m, *this, std::less<int>());
            }
            Trivial operator>=(const Member& m) {
                return trivialLessThan(m, *this, std::less_equal<int>());
            }
            
        private:
            std::vector<double> addVariables(Member m1, Member m2){
                int size = std::max(m1.variables.size(), m2.variables.size());
                std::vector<double> res(size);
                m1.variables.resize(size, 0);
                m2.variables.resize(size, 0);
                std::transform(m1.variables.begin(),m1.variables.end(),m2.variables.begin(),res.begin(),std::plus<double>());
                return res;
            }

            std::vector<double> subtractVariables(Member m1, Member m2){
                int size = std::max(m1.variables.size(), m2.variables.size());
                std::vector<double> res(size);
                m1.variables.resize(size, 0);
                m2.variables.resize(size, 0);
                std::transform(m1.variables.begin(),m1.variables.end(),m2.variables.begin(),res.begin(),std::minus<double>());
                return res;
            }

            std::vector<double> multiply(Member m1, Member m2){
                std::vector<double> res;
                if(m1.isConstant() != m2.isConstant()){
                    if(!m1.isConstant()){
                        for(auto& v : m1.variables){
                            res.push_back(v * m2.constant);
                        }
                    } else if(!m2.isConstant()){
                        for(auto& v : m2.variables){
                            res.push_back(v * m1.constant);
                        }
                    }
                }
                return res;
            }
            
            Trivial trivialLessThan(Member m1, Member m2, std::function<bool (int, int)> compare){
                MemberType type1 = m1.getType();
                MemberType type2 = m2.getType();
                
                // self comparison
                if(m1 == m2)
                    return compare(m1.constant, m2.constant) ? Trivial::True : Trivial::False;
                    
                // constant < constant/input/output
                if(type1 == MemberType::Constant){
                    if(type2 == MemberType::Constant){
                        return compare(m1.constant, m2.constant) ? Trivial::True : Trivial::False;
                    }
                    else if(type2 == MemberType::Input && !compare(m1.constant, m2.constant)){
                        return Trivial::False;
                    } 
                    else if(type2 == MemberType::Output && compare(m1.constant, m2.constant)){
                        return Trivial::True;
                    } 
                } 
                // input < output/constant
                else if(type1 == MemberType::Input 
                        && (type2 == MemberType::Constant || type2 == MemberType::Output) 
                        && compare(m1.constant, m2.constant)) {
                    return Trivial::True;
                } 
                // output < input/constant
                else if(type1 == MemberType::Output 
                        && (type2 == MemberType::Constant || type2 == MemberType::Input) 
                        && !compare(m1.constant, m2.constant)) {
                    return Trivial::False;
                } 
                return Trivial::Indeterminate;
            }

        };
    }
}

#endif /* MEMBER_H */
