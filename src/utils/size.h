#ifndef VOXEL_SIZE_H
#define VOXEL_SIZE_H

#include <cstddef>
#include <stdexcept>

/**
 * Collection of various constexpr utils, currently only for handling array size-related stuff.
 */
namespace SizeUtils {
    constexpr size_t pow(const size_t x, const size_t p) {
        if (p == 0) return 1;
        if (p == 1) return x;

        const size_t tmp = pow(x, p / 2);
        if (p % 2 == 0)
            return tmp * tmp;
        return x * tmp * tmp;
    }

    constexpr size_t log(const size_t base, const size_t x) {
        if (base <= 1) {
            throw std::runtime_error("invalid base in SizeUtils::log");
        }

        size_t res = 0;
        size_t currPow = 1;

        while (currPow < x) {
            currPow *= base;
            res++;
        }

        return res;
    }
}

#endif //VOXEL_SIZE_H
