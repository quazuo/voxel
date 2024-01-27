#ifndef VOXEL_SIZE_H
#define VOXEL_SIZE_H

#include <cstddef>
#include <array>

/**
 * Collection of various constexpr utils, currently only for handling array size-related stuff.
 */
namespace SizeUtils {
    constexpr size_t pow(const size_t x, const size_t p)
    {
        if (p == 0) return 1;
        if (p == 1) return x;

        size_t tmp = pow(x, p / 2);
        if (p % 2 == 0)
            return tmp * tmp;
        return x * tmp * tmp;
    }
}

#endif //VOXEL_SIZE_H
