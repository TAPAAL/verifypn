#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <ctime>
#include <sstream>

using namespace std;

class stopwatch {
    bool _running = false;
    clock_t _start;
    clock_t _stop;

public:
    double started() const { return _start; }
    double stopped() const { return _stop; }
    bool running() const { return _running; }
    void start() {
        _running = true;
        _start = clock();
    }
    void stop() {
        _stop = clock();
        _running = false;
    }
    double duration() const { return ( (double(_stop - _start))*1000)/CLOCKS_PER_SEC; }

    ostream &operator<<(ostream &os){
        os << duration() << " ms";
        return os;
    }

    std::string toString(){
        stringstream ss;
        ss << this;
        return ss.str();
    }
};

#endif // STOPWATCH_H
