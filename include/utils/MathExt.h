#ifndef MATHEXT_H
#define MATHEXT_H

//
template <typename S, typename U>
U signed_wrap(S value, U max) {
    return ((value % max) + max) % max;
}
#endif //MATHEXT_H
