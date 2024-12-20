#ifndef MATHEXT_H
#define MATHEXT_H

/// maps value to [0,max) using modulo but when value is negative
/// it is handled differently s.t. signed_wrap(-1, 5) = 4
template <typename S, typename U>
U signed_wrap(S value, U max) {
    return ((value % max) + max) % max;
}
#endif //MATHEXT_H
