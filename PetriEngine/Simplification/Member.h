#ifndef MEMBER_H
#define MEMBER_H
#include <algorithm>

namespace PetriEngine {
    namespace Simplification {
        
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
        };
    }
}

#endif /* MEMBER_H */
