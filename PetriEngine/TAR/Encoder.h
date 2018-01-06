// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 1995 - 2017, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef ENCODER_H
#define ENCODER_H
//#define CONSTPARAM
#include <vector>
#include <memory>

#include "z3++.h"

#include "system/System.h"
#include "system/Edge.h"
#include "ConstraintVisitor.h"

class Encoder
{
private:
    z3::context& context;
    const size_t size;
    std::unique_ptr<size_t[]> names;
    std::vector<z3::expr> vars;
    z3::expr delvar;
    const size_t delayindex;
    System& sys;
    std::unique_ptr<bool[]> rclocks;
    std::vector<z3::expr> rates;
public:
    std::vector<int> constants;

    Encoder(z3::context& context, System& system) 
    :       context(context), 
            size(system.getVariableTableSize() + 1), 
            names(std::make_unique<size_t[]>(size)),
            vars(),
            delvar(context.bool_val(false)),
            delayindex(size - 1),
            sys(system),
            rclocks(std::make_unique<bool[]>(system.getNrOfClocks()))
    {
        reset();
    }
    
    virtual ~Encoder() {

    }
    
    z3::expr& delay_var()
    {
        return delvar;
    }
    
    z3::context& get_context()
    {
        return context;
    }
        
    void reset()
    {
        memset(names.get(), 0, sizeof(size_t)*size);
        std::string dn = to_string(delayindex) + "~r" + to_string(names[delayindex]);
        delvar = context.real_const(dn.c_str());       

        vars.clear();

        for(size_t i = 0; i < sys.getVariableTableSize(); ++i)
        {
            while(vars.size() < i) vars.push_back(context.bool_val(false));
            auto& v = sys.getVariable(i);
            if(auto clock = dynamic_cast<ClockVariable*>(v.get()))
            {
                if( clock->getValue(NULL) == 0 || // TAU
                    (cindex_t)clock->getValue(NULL) == sys.getGlobalTimeClock()) continue; // GTC, we dont need!
                std::string name = to_string(i) + "~r" + to_string(names[i]);
                vars.push_back(context.real_const( name.c_str()));
            }
            else if (auto integer = dynamic_cast<StateVariable*>(v.get()))
            {
                if(integer->isConstant()) continue;
                if(integer->getName()[0] == '#') continue;
                if(v->getMinimum() == 0 && v->getMaximum() == 1)
                {
                    std::string name = to_string(i) + "~b" + to_string(names[i]);
                    vars.push_back(context.bool_const(name.c_str()));
                }
                else
                {
                    std::string name = to_string(i) + "~i" + to_string(names[i]);
                    vars.push_back(context.int_const( name.c_str()));
                }
            }
        }
        
        for(size_t i = 0; i < sys.getNrOfClocks(); ++i)
        {
            rates.push_back(context.real_val(1));
        }
        
        for(const System::param_t& p : sys.getParameters())
        {
            if(p.var->getType() == Variable::FLOATINGPOINT ||
                    p.var->getType() == Variable::SYSTEMFLOATING)
            {
                vars[p.var->getIndex()] = context.real_const(p.name.c_str());
            }
            else
            {
                vars[p.var->getIndex()] = context.int_const(p.name.c_str());
            }
        }
    }
    
    z3::expr encode_invariant(const Location* loc)
    {
        ConstraintVisitor visitor(sys, context, delvar, names.get(), vars, rates);
        if(visitor.do_visit(loc->astinv, loc->getProcess()) && visitor.result.size() > 0)
        {
#ifdef CONSTPARAM
            std::vector<int> buffer;
            std::set_union(constants.begin(), constants.end(), 
                    visitor.constants.begin(), visitor.constants.end(), 
                    std::back_inserter(buffer));
            constants.swap(buffer);
#endif
            if(!visitor.result.back().is_bool())
                visitor.result.back() = (visitor.result.back() != 0);
            return visitor.result.back().simplify();
        }
        return context.bool_val(true);
    }
    
    z3::expr encode_invariant(SymbolicState& state)
    {
        for(size_t i = 0; i < sys.getNrOfClocks(); ++i) rates[i] = context.real_val(1);
        ConstraintVisitor visitor(sys, context, delvar, names.get(), vars, rates);
        z3::expr cons_sys = context.bool_val(true);
        bool urgent = false;
        for(size_t p = 0; p < sys.getNrOfProcesses(); ++p)
        {
            if(visitor.do_visit(state.getLocation(p)->astinv, p) && visitor.result.size() > 0)
            {
                if(!visitor.result.back().is_bool())
                    visitor.result.back() = (visitor.result.back() != 0);
                cons_sys = (cons_sys && visitor.result.back());
            }
            urgent = urgent || state.getLocation(p)->isCommitted() || state.getLocation(p)->isUrgent();
        }
        if(urgent)
        {
            for(size_t i = 0; i < sys.getNrOfClocks(); ++i) rates[i] = context.real_val(0);
        }
#ifdef CONSTPARAM
        std::vector<int> buffer;
        std::set_union(constants.begin(), constants.end(), 
                visitor.constants.begin(), visitor.constants.end(), 
                std::back_inserter(buffer));
        constants.swap(buffer);
#endif
        return cons_sys.simplify();
    }
    
    z3::expr encode_edge(const Edge* cedge, bool only_guard = false)
    {
        memset(rclocks.get(), 0, sizeof(bool)*sys.getNrOfClocks());
        auto cons_sys = context.bool_val(true);
        ConstraintVisitor visitor(sys, context, delvar, names.get(), vars, rates);

        {
            if(visitor.do_visit(cedge->astguard, cedge->getProcessNr()))
            {
                if(!visitor.result.back().is_bool())
                    visitor.result.back() = (visitor.result.back() != 0);
                cons_sys = cons_sys && visitor.result.back();
            }
        }
        if(only_guard) return cons_sys.simplify();
        {
            cons_sys = cons_sys && encode_update(cedge);
        }        
#ifdef CONSTPARAM
        std::vector<int> buffer;
        std::set_union(constants.begin(), constants.end(), 
                visitor.constants.begin(), visitor.constants.end(), 
                std::back_inserter(buffer));
        constants.swap(buffer);
#endif
        return cons_sys.simplify();
    }
    
    z3::expr encode_update(const Edge* cedge)
    {
        ConstraintVisitor visitor(sys, context, delvar, names.get(), vars, rates);
        auto cons_sys = context.bool_val(true);
        if(visitor.do_visit(cedge->astupdate, cedge->getProcessNr(), rclocks.get()))
        {
            return visitor.side_effect.simplify();
        }
        return cons_sys.simplify();
    }
    
    z3::expr encode_background()
    {
        z3::expr csys = context.bool_val(true);
        for(size_t i = 0; i < sys.getNrOfClocks(); ++i)
        {
            Variable_ptr clock = sys.getClock(i);
            size_t cid = clock->getValue(NULL);
            if( cid == 0 || // TAU
                cid == sys.getGlobalTimeClock()) continue; // GTC, we dont need!
            
            for(size_t i = 0; i < names[clock->getIndex()]; ++i)
            {
                std::string nname = to_string(clock->getIndex()) + "~r" + to_string(names[clock->getIndex()]);
                csys = csys && context.real_const(nname.c_str()) >= 0;
            }    
        }
        
        for(size_t i = 0; i < names[delayindex]; ++i)
        {
            std::string delname = to_string(delayindex) + "~r" + to_string(names[delayindex]);            
            csys = csys && context.real_const(delname.c_str()) >= 0;
        }
        
        return csys;
    }
    
    z3::expr encode_clocks()
    {
        std::string delname = to_string(delayindex) + "~r" + to_string(names[delayindex]);
        delvar = context.real_const(delname.c_str());
        z3::expr cons_sys = delvar >= 0;

        for(size_t i = 0; i < sys.getNrOfClocks(); ++i)
        {

            if(rclocks[i]) continue;
            Variable_ptr clock = sys.getClock(i);
            size_t cid = clock->getValue(NULL);
            if( cid == 0 || // TAU
                cid == sys.getGlobalTimeClock()) continue; // GTC, we dont need!
            auto oclk = vars[clock->getIndex()];
            int r = 0;
            bool is_numeral = rates[cid].is_numeral_i(r);
            if(is_numeral && r == 0) continue;

            names[clock->getIndex()] += 1;
            std::string nname = to_string(clock->getIndex()) + "~r" + to_string(names[clock->getIndex()]);
            vars[clock->getIndex()] = context.real_const(nname.c_str());
            cons_sys = 
                    (cons_sys && 
                        (vars[clock->getIndex()] == // new clock variable
                            (oclk // old var
                            +   (is_numeral && r == 1 ? 
                                    delvar : 
                                    delvar * rates[cid]
                                ) // delay
                            )
                        )
                    );            
        }
        ++names[delayindex];
        return cons_sys.simplify();
    }
    
    void encode_vars(z3::expr_vector& vars, const bool last = true, const bool first = true)
    {
        for(size_t i = 0; i < sys.getVariableTableSize(); ++i)
        {
            auto& v = sys.getVariable(i);
            bool isParam = false;
            for(const auto& p : sys.getParameters())
            {
                if(v == p.var)
                {
                    isParam = true;
                    break;
                }
            }
            if(isParam) continue; // Free variable!
            if(auto clock = dynamic_cast<ClockVariable*>(v.get()))
            {
                if( clock->getValue(NULL) == 0 || // TAU
                    (cindex_t)clock->getValue(NULL) == sys.getGlobalTimeClock()) continue; // GTC, we dont need!
                
                for(size_t j = (first ? 0 : 1) ; (last ? j <= names.get()[i] :
                                                         j <  names.get()[i] ); ++j)
                {
                    vars.push_back(context.real_const(ConstraintVisitor::make_name(v, j).c_str()));
                }
            }
            else if (auto integer = dynamic_cast<StateVariable*>(v.get()))
            {
                if(integer->isConstant()) continue;
                if(integer->getName()[0] == '#') continue;
                for(size_t j = (first ? 0 : 1); (last ? j <= names.get()[i] :
                                                        j <  names.get()[i] ); ++j)
                {
                    if(v->getMinimum() == 0 && v->getMaximum() == 1)
                    {
                        vars.push_back(context.bool_const(ConstraintVisitor::make_name(v, j).c_str()));
                    }
                    else
                    {
                        vars.push_back(context.int_const(ConstraintVisitor::make_name(v, j).c_str()));
                    }
                }
            }   
        }
        for(size_t j = 0; j <= names.get()[delayindex]; ++j)
        {
            auto name = to_string(delayindex) + "~r" + to_string(j);
            vars.push_back(context.real_const(name.c_str()));
        }
    }
  
    z3::expr encode_initial(SymbolicState& state)
    {
        auto cons_sys = delvar >= 0;

        cons_sys = cons_sys && encode_invariant(state);
        for(size_t i = 0; i < sys.getVariableTableSize(); ++i)
        {
            auto& v = sys.getVariable(i);
            bool isParam = false;
            for(const auto& p : sys.getParameters())
            {
                if(v == p.var)
                {
                    isParam = true;
                    break;
                }
            }
            if(isParam) continue; // Free variable!
            if(auto clock = dynamic_cast<ClockVariable*>(v.get()))
            {
                size_t cindex = clock->getValue(NULL);
                if( cindex == 0 || // TAU
                    cindex == sys.getGlobalTimeClock()) continue; // GTC, we dont need!
                cons_sys = cons_sys && (vars[i] == (delvar * rates[cindex]));
            }
            else if (auto integer = dynamic_cast<StateVariable*>(v.get()))
            {
                if(integer->isConstant()) continue;
                if(integer->getName()[0] == '#') continue;
                if(v->getMinimum() == 0 && v->getMaximum() == 1)
                {
                    if((v->getInitialValue() == 0))
                    {
                        cons_sys = cons_sys && (!vars[i]);
                    }
                    else
                    {
                        cons_sys = cons_sys && vars[i];
                    }
                }
                else
                {
                    cons_sys = cons_sys && (vars[i] == v->getInitialValue());
                }
            }
        }
        ++names[delayindex];
        std::string dn = to_string(delayindex) + "~r" + to_string(names[delayindex]);
        delvar = context.real_const(dn.c_str());    
        return cons_sys.simplify();
    }
    
    size_t max_id()
    {
        size_t mid = 0;
        for(size_t i = 0; i < size; ++i)
        {
            mid = std::max(mid, names[i]);
        }
        return mid;
    }
    
    const size_t* get_names()
    {
        return names.get();
    }
    
    std::vector<z3::expr>& getVars()
    {
        return vars;
    }
};

#endif /* ENCODER_H */

