/*
 * File:   shared_string.h
 * Author: Peter G. Jensen
 *
 * Created on 28 March 2022, 14.59
 */

#ifndef SHARED_STRING_H
#define SHARED_STRING_H

#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>

typedef const std::string const_string;
typedef std::shared_ptr<const_string> shared_const_string;

template<typename T>
struct shared_ops {

    bool operator()(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b) const {
        return *a == *b;
    }

    // hasher
    std::size_t operator()(const std::shared_ptr<T>& str) const {
        using non_const = typename std::remove_const<T>::type;
        return std::hash<non_const>{}(*const_cast<non_const*>(str.get()));
    }
};

typedef std::unordered_map<shared_const_string, uint32_t,
                        shared_ops<const_string>, shared_ops<const_string>> shared_name_index_map;

typedef std::unordered_map<shared_const_string, std::vector<shared_const_string>,
shared_ops<const_string>, shared_ops<const_string>> shared_name_name_map;

typedef std::unordered_map<shared_const_string, std::unordered_map<uint32_t, shared_const_string>,
            shared_ops<const_string>, shared_ops<const_string>> shared_place_color_map;

typedef std::unordered_set<shared_const_string, shared_ops<const_string>, shared_ops<const_string>> shared_string_set;


#endif /* SHARED_STRING_H */

