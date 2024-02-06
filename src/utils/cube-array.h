#ifndef VOXEL_CUBE_ARRAY_H
#define VOXEL_CUBE_ARRAY_H

#include <cstddef>
#include <array>
#include <functional>

/**
 * Helper structure wrapping a nested 3-dimensional std::array of equal sizes known at compile-time.
 *
 * @tparam T Element type.
 * @tparam S Size of the arrays.
 */
template<typename T, size_t S>
class CubeArray {
    std::array<std::array<std::array<T, S>, S>, S> arr;

public:
    explicit CubeArray(T defaultValue = T()) {
        map([&](T x) -> T {
            (void) x;
            return defaultValue;
        });
    }

    const std::array<std::array<T, S>, S>& operator[](const size_t n) const {
        return arr[n];
    }

    std::array<std::array<T, S>, S>& operator[](const size_t n) {
        return arr[n];
    }

    const T& operator[](const glm::ivec3& vec) const {
        return arr[vec.x][vec.y][vec.z];
    }

    T& operator[](const glm::ivec3& vec) {
        return arr[vec.x][vec.y][vec.z];
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

/**
 * Helper structure wrapping a nested 3-dimensional std::vector of equal sizes unknown at compile-time.
 *
 * @tparam T Element type.
 */
template<typename T>
class CubeVector {
    size_t currSize = 0;
    std::vector<std::vector<std::vector<T>>> arr;

public:
    explicit CubeVector(const size_t size = 0, T defaultValue = T()) : currSize(size) {
        for (size_t x = 0; x < size; x++) {
            std::vector<std::vector<T>> v;

            for (size_t y = 0; y < size; y++) {
                v.emplace_back(size, defaultValue);
            }

            arr.push_back(v);
        }
    }

    const std::vector<std::vector<T>>& operator[](const size_t n) const {
        return arr[n];
    }

    std::vector<std::vector<T>>& operator[](const size_t n) {
        return arr[n];
    }

    const T& operator[](const glm::ivec3& vec) const {
        return arr[vec.x][vec.y][vec.z];
    }

    T& operator[](const glm::ivec3& vec) {
        return arr[vec.x][vec.y][vec.z];
    }

    void forEach(std::function<void(size_t, size_t, size_t, T &)> f) {
        for (size_t x = 0; x < currSize; x++) {
            for (size_t y = 0; y < currSize; y++) {
                for (size_t z = 0; z < currSize; z++) {
                    f(x, y, z, arr[x][y][z]);
                }
            }
        }
    }

    void map(std::function<T(T)> f) {
        for (size_t x = 0; x < currSize; x++) {
            for (size_t y = 0; y < currSize; y++) {
                for (size_t z = 0; z < currSize; z++) {
                    arr[x][y][z] = f(arr[x][y][z]);
                }
            }
        }
    }
};

#endif //VOXEL_CUBE_ARRAY_H
