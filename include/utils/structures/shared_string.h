/*
 * File:   shared_string.h
 * Author: Peter G. Jensen
 *
 * Created on 28 March 2022, 14.59
 */

#ifndef SHARED_STRING_H
#define SHARED_STRING_H

struct shared_string_ops {

    bool operator()(const std::shared_ptr<std::string>& a, const std::shared_ptr<std::string>& b) const {
        return *a == *b;
    }

    // hasher

    std::size_t operator()(const std::shared_ptr<std::string>& str) const {
        return std::hash<std::string>{}(*str);
    }
};

typedef std::unordered_map<std::shared_ptr<std::string>, uint32_t,
shared_string_ops, shared_string_ops> shared_name_index_map;

typedef std::unordered_map<std::shared_ptr<std::string>, std::vector<std::shared_ptr<std::string>>,
shared_string_ops, shared_string_ops> shared_name_name_map;

typedef std::unordered_map<std::shared_ptr<std::string>, std::unordered_map<uint32_t , std::shared_ptr<std::string>>,
            shared_string_ops, shared_string_ops> shared_place_color_map;


#endif /* SHARED_STRING_H */

