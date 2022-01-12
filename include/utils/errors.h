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

#endif /* ERRORS_H */
