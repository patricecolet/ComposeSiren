//
// Created by joseph larralde on 02/03/2026.
//

#ifndef COMPOSESIREN_MATH_UTILS_H
#define COMPOSESIREN_MATH_UTILS_H

#include <cmath>
#include <concepts>

template <std::floating_point T>
T dbtoa(T db) {
    return pow(10, db / 20);
}

template <std::floating_point T>
T atodb(T a) {
    return 20 * log10(a);
}

template <std::floating_point T> // C++20 minimum for concepts
T linearMap(T in, T minin, T maxin, T minout, T maxout) {
    T epsilon = static_cast<T>(1e-9);
    if (abs(maxin - minin) < epsilon)
        // not very satisfying but this is a pathological case anyway
            return (minout + maxout) * 0.5;
    if (abs(maxout - minout) < epsilon)
        // this makes more sense
            return maxout;
    T a = (maxout - minout) / (maxin - minin);
    T b = maxout - a * maxin;
    return a * in + b;
}

#endif // COMPOSESIREN_MATH_UTILS_H
