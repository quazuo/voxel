#ifndef VOXEL_SIZE_H
#define VOXEL_SIZE_H

#include <cstddef>
#include <array>

namespace SizeUtils {
    template<typename T, size_t S>
    using CubeArray = std::array<std::array<std::array<T, S>, S>, S>;

    constexpr size_t powSize(const size_t x, const size_t p)
    {
        if (p == 0) return 1;
        if (p == 1) return x;

        size_t tmp = powSize(x, p / 2);
        if (p % 2 == 0)
            return tmp * tmp;
        return x * tmp * tmp;
    }

}

#endif //VOXEL_SIZE_H
