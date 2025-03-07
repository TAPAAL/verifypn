#ifndef MATHEXT_H
#define MATHEXT_H

/// maps value to [0,max) using modulo but when value is negative
/// it is handled differently s.t. signed_wrap(-1, 5) = 4
template <typename S, typename U>
std::make_unsigned_t<U> signed_wrap(S value, U max) {
    static_assert(std::numeric_limits<S>::min() >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<S>::max() <= std::numeric_limits<int64_t>::max(), "S needs to be containable in int64_t");
    static_assert(std::numeric_limits<U>::min() >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<U>::max() <= std::numeric_limits<int64_t>::max(), "U needs to be containable in int64_t");
    static_assert(std::is_signed_v<S>, "S needs to be signed");
    return static_cast<U>(((static_cast<int64_t>(value) % static_cast<int64_t>(max)) + static_cast<int64_t>(max)) % static_cast<int64_t>(max));
}

template<typename U, typename O>
std::make_unsigned_t<U> add_color_offset(U baseValue, O offset, U max) {
    static_assert(std::numeric_limits<U>::min() >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<U>::max() <= std::numeric_limits<int64_t>::max(), "U needs to be containable in int64_t");
    static_assert(std::numeric_limits<O>::min() >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<O>::max() <= std::numeric_limits<int64_t>::max(), "O needs to be containable in int64_t");
    static_assert(std::is_signed_v<O>, "O needs to be signed");
    return signed_wrap(static_cast<int64_t>(baseValue) + static_cast<int64_t>(offset), max);
}
#endif //MATHEXT_H
