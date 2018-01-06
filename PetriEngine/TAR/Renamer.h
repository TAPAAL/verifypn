// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 1995 - 2017, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RENAMER_H
#define RENAMER_H

#include <vector>

#include "z3++.h"

class Renamer 
{
private:
    z3::context& context;
    z3::expr_vector from;
    z3::expr_vector to;
    bool false_const = false;
    std::vector<size_t> seen;
    bool do_rename = true;
    bool contains_real = false;
    
    void visit(z3::expr node, const size_t* indexes)
    {
        false_const = false;
        if(node.is_const())
        {
            if(node.kind() == Z3_APP_AST)
            {
                stringstream rnss;
                rnss << node;
                std::string oldname = rnss.str();
                if(oldname == "false")
                {
                    false_const = true;
                    return;
                } 
                else if(oldname == "true")
                {
                    return;
                }
                else if(oldname[0] == '~') // constant or epsilon
                {
                    return;
                }
                oldname = oldname.substr(1);
                std::string newname = oldname;
                size_t index = oldname.find_last_of("~");
                if(index == std::string::npos) return; // parameter!
                newname.erase(newname.begin() + (index + 2), newname.end());
                if(indexes == NULL)
                {
                    newname += to_string(0);
                    newname[index] = 0;
                    size_t varid = 0;
                    sscanf(newname.c_str(), "%zu", &varid);
                    newname[index] = '~';
                    auto lb = std::lower_bound(seen.begin(), seen.end(), varid);
                    if(lb == seen.end() || *lb != varid) seen.insert(lb, varid);
                }
                else
                {
                    newname[index] = 0;
                    size_t varid = 0;
                    sscanf(newname.c_str(), "%zu", &varid);
                    newname[index] = '~';
                    newname += to_string(indexes[varid]);
                }
                
                if(newname[index + 1] == 'r')
                {
                    contains_real = true;
                }

                if(!do_rename) return;
                
                from.push_back(node);
                switch(newname[index + 1])
                {
                    case 'i':
                        to.push_back(context.int_const(newname.c_str()));
                        break;
                    case 'b':
                        to.push_back(context.bool_const(newname.c_str()));
                        break;
                    case 'r':
                        to.push_back(context.real_const(newname.c_str()));
                        break;
                    default:
                        std::cerr << "UNKNOWN NODE TYPE" << std::endl;
                        assert(false);
                        exit(-1);
                        break;
                }              
                               
            }
        }
        else
        {
            for(size_t i = 0; i < node.num_args(); ++i)
            {
                visit(node.arg(i), indexes);
            }
        }
    }
    
public:
    Renamer(z3::context& context) : context(context), from(context), to(context)
    {}

    void contains(z3::expr expression)
    {
        contains_real = false;
        do_rename = false;
        seen.clear();
        visit(expression, NULL);
    }
    
    z3::expr rename(z3::expr expression, const size_t* indexes = NULL)
    {
        contains_real = false;
        do_rename = true;
        seen.clear();
        false_const = false;
        from.resize(0);
        to.resize(0);
        visit(expression, indexes);
        
        return expression.substitute(from, to);
    }
    
    std::vector<size_t>& variables_seen()
    {
        return seen;
    }

    
    bool has_real() { return contains_real; };
};
#endif /* RENAMER_H */

