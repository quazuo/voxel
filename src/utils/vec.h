#ifndef VOXEL_VEC_H
#define VOXEL_VEC_H

#include <string>
#include <functional>
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

/**
 * Collection of various utils for handling GLM 3-dimensional vectors.
 */
namespace VecUtils {
    struct VecHash {
        size_t operator()(const glm::vec3 &a) const {
            const size_t h1 = std::hash<double>()(a.x);
            const size_t h2 = std::hash<double>()(a.y);
            const size_t h3 = std::hash<double>()(a.z);
            return (h1 ^ (h2 << 1)) ^ h3;
        }
    };

    template<typename T>
    T sum(const glm::vec<3, T> &v) {
        return v.x + v.y + v.z;
    }

    template<typename T>
    std::string toString(const glm::vec<3, T> &vec) {
        std::stringstream ss;
        ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
        return ss.str();
    }

    template<typename T>
    using VecPredicate = std::function<bool(T)>;
    template<typename T>
    using VecFunctor = std::function<T(T)>;

    template<typename T>
    bool all(const glm::vec<3, T> &vec, const VecPredicate<T>& pred) {
        return pred(vec.x) && pred(vec.y) && pred(vec.z);
    }

    template<typename T>
    bool any(const glm::vec<3, T> &vec, const VecPredicate<T>& pred) {
        return pred(vec.x) || pred(vec.y) || pred(vec.z);
    }

    template<typename T>
    glm::vec<3, T> map(glm::vec<3, T> vec, const VecFunctor<T>& f) {
        vec.x = f(vec.x);
        vec.y = f(vec.y);
        vec.z = f(vec.z);
        return vec;
    }

    template<typename T>
    glm::vec3 floor(const glm::vec<3, T> &vec) {
        static const VecFunctor f = [](const T x) { return std::floor(x); };
        return map<T>(vec, f);
    }

    template<typename T>
    glm::vec3 abs(const glm::vec<3, T> &vec) {
        static const VecFunctor f = [](const T x) { return std::abs(x); };
        return map<T>(vec, f);
    }
}

#endif //VOXEL_VEC_H
