/* 
 * File:   Renamer.h
 * Author: srba
 *
 * Created on 22 March 2014, 16:30
 */

#ifndef RENAMER_H
#define	RENAMER_H

#include <stdio.h>
#include <string>

namespace Renamer {
    
void removeWildCharacters(std::string &text);

void addWildCharacters(std::string &text);

}

#endif	/* RENAMER_H */

