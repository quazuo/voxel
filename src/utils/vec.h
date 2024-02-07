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
    int sum(const glm::ivec3 &v);

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

    glm::vec3 floor(const glm::vec3 &vec);

    glm::vec3 abs(const glm::vec3 &vec);
}

#endif //VOXEL_VEC_H
