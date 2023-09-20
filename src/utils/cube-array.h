#ifndef VOXEL_CUBE_ARRAY_H
#define VOXEL_CUBE_ARRAY_H

#include <cstddef>
#include <array>
#include <functional>

template<typename T, size_t S>
struct CubeArray {
    std::array<std::array<std::array<T, S>, S>, S> arr;

    explicit CubeArray(T defaultValue = T()) {
        init(defaultValue);
    }

    const std::array<std::array<T, S>, S>& operator[](size_t n) const {
        return arr[n];
    }

    std::array<std::array<T, S>, S>& operator[](size_t n) {
        return arr[n];
    }

    const T& operator[](VecUtils::Vec3Discrete vec) const {
        return arr[vec.x][vec.y][vec.z];
    }

    T& operator[](VecUtils::Vec3Discrete vec) {
        return arr[vec.x][vec.y][vec.z];
    }

    void init(T defaultValue = T()) {
        map([&](T x) -> T {
            (void) x;
            return defaultValue;
        });
    }

    void forEach(std::function<void(size_t, size_t, size_t, T &)> f) {
        for (size_t x = 0; x < S; x++) {
            for (size_t y = 0; y < S; y++) {
                for (size_t z = 0; z < S; z++) {
                    f(x, y, z, arr[x][y][z]);
                }
            }
        }
    }

    void map(std::function<T(T)> f) {
        for (size_t x = 0; x < S; x++) {
            for (size_t y = 0; y < S; y++) {
                for (size_t z = 0; z < S; z++) {
                    arr[x][y][z] = f(arr[x][y][z]);
                }
            }
        }
    }
};

#endif //VOXEL_CUBE_ARRAY_H
