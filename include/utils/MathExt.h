#ifndef MATHEXT_H
#define MATHEXT_H

/// maps value to [0,max) using modulo but when value is negative
/// it is handled differently s.t. signed_wrap(-1, 5) = 4
template <typename S, typename U>
std::make_unsigned_t<U> signed_wrap(S value, U max) {
    static_assert(std::is_signed_v<S> && std::is_signed_v<U>, "Value and max must be signed");
    return ((value % max) + max) % max;
}
#endif //MATHEXT_H
