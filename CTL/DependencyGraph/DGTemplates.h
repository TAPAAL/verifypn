#ifndef PARTIAL_H
#define PARTIAL_H

#include "AbstractDependencyGraphs.h"
#include "Configuration.h"
#include <mutex>
#include <unordered_set>
#include <functional>

namespace DependencyGraph{

template<class T>
class TConfiguration : public Configuration {
public:
    TConfiguration() {}
    TConfiguration(T &t_t) { t = t_t; }
    TConfiguration(T &&t_t) { t = t_t; }
    T t;
};

template<class T,
         class hash = std::hash<T>,
         class pred = std::equal_to<T>,
         class alloc = std::allocator<T>>
class DependencyGraphTemplate : public virtual BasicDependencyGraph {
public:
    typedef TConfiguration<T> config_type;
    typedef config_type* config_pointer;

//private:

    //config_type equality for the config_set
    struct TConfig_equal_to{
        pred comparer;
        bool operator ()(config_type const &l, config_type const &r) const {
            return comparer.operator() (l.t, r.t);
        }
    };
    struct Tconfig_hash{
        hash hasher;
        std::size_t operator ()(config_type const &tc) const {
            return hasher.operator()(tc.t);
        }
    };
protected:
    void clear() { configurations.clear();}
public:
    std::unordered_set<config_type, Tconfig_hash, TConfig_equal_to, alloc> configurations;

    T* getT(Configuration &c){
        return &(static_cast<config_pointer>(&c)->t);
    }

    std::pair<config_pointer, bool> getConfiguration(T &t){
        auto pair = configurations.emplace(t);
        config_pointer tc = const_cast<config_type*>(&*pair.first);
        return std::make_pair(tc, pair.second);
    }

    Edge* makeSuccessor(T &t){
        Configuration *c = getConfiguration(t).first;
        Edge *e = new Edge(*c);
        c->successors.push_back(e);
        return e;
    }

    void addSuccessor(T &&t, Edge &e){
        Configuration *c = getConfiguration(t).first;
        c->successors.push_back(&e);
    }

    void addTarget(Edge &e, T &t){
        Configuration *c = getConfiguration(t).first;
        e.targets.push_back(c);
    }
};
/*
template<class T,
         class lockable,
         class hash = std::hash<T>,
         class pred = std::equal_to<T>,
         class alloc = std::allocator<T>>
class ConcurrentDependencyGraphTemplate : public virtual DependencyGraphTemplate<T, hash, pred, alloc> {
protected:
    //mutex for locking the set configurations
    std::mutex config_mtx;
public:
    typedef typename DependencyGraphTemplate<T, hash, pred, alloc>::config_type config_type;
    typedef typename DependencyGraphTemplate<T, hash, pred, alloc>::config_pointer config_pointer;

    //override get_configuration, as to lock before altering configurations
    std::pair<config_pointer, bool> getConfiguration(T &t){
        lockable(config_mtx);
        return getConfiguration(t);
    }
};*/
}

#endif // PARTIAL_H
