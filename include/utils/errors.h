/*
 * Copyright (C) 2020 Peter G. Jensen <root@petergjoel.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File:   errors.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on May 10, 2019, 9:46 AM
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <sstream>
#include <limits>
#include <cstdint>
#include <charconv>
#include <string_view>
#include <system_error>

enum class ReturnValue {
    SuccessCode = 0,
    FailedCode = 1,
    UnknownCode = 2,
    ErrorCode = 3,
    ContinueCode = 4
};

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

// TODO: Shared this with other projects
struct base_error : public std::exception {
    std::string _message;

    template<typename ...Args>
    base_error(Args ...args) {
        std::stringstream ss;
        (ss << ... << args);
        _message = ss.str();
    }

    const char *what() const noexcept override {
        return _message.c_str();
    }

    virtual void print(std::ostream &os) const {
        os << what() << std::endl;
    }

    friend std::ostream &operator<<(std::ostream &os, const base_error &el) {
        el.print(os);
        return os;
    }
};

inline uint32_t toBoundedTokenCount(uint64_t count) {
    if (count > std::numeric_limits<uint32_t>::max()) {
        throw base_error("Number of tokens exceeded ", std::numeric_limits<uint32_t>::max());
    }
    return static_cast<uint32_t>(count);
}

inline int32_t parse_bounded_int32(std::string_view text) {
    if (text.empty()) {
        throw base_error("Expected an integer constant, got \"\"");
    }

    int32_t value = 0;
    const char* const first = text.data();
    const char* const last = first + text.size();
    const std::from_chars_result result = std::from_chars(first, last, value);
    if (result.ec == std::errc::invalid_argument || result.ptr == first || result.ptr != last) {
        throw base_error("Expected an integer constant, got \"", text, "\"");
    }
    if (result.ec == std::errc::result_out_of_range) {
        if (text.front() == '-') {
            throw base_error("Integer constant ", text, " is less than ",
                             std::numeric_limits<int32_t>::min());
        }
        throw base_error("Integer constant ", text, " exceeded ",
                         std::numeric_limits<int32_t>::max());
    }
    return value;
}

#endif /* ERRORS_H */
