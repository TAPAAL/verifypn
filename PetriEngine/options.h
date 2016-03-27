#ifndef OPTIONS_H

#include <ctype.h>
#include <stddef.h>
#include <limits>

struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    char* modelfile = NULL;
    char* queryfile = NULL;
    bool disableoverapprox = false;
    int enablereduction = 0; // 0 ... disabled (default),  1 ... aggresive, 2 ... k-boundedness preserving
    bool statespaceexploration = false;
    bool printstatistics = true;
    size_t memorylimit = 2048;
    size_t querynumber = std::numeric_limits<size_t>::max();
};

#endif