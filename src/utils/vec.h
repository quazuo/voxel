#ifndef VOXEL_VEC_H
#define VOXEL_VEC_H

#include <string>
#include <functional>
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

namespace VecUtils {
    using Vec3Discrete = glm::vec<3, int>;

    int sum(Vec3Discrete v);

    std::string toString(glm::vec3 vec);

    using VecPredicate = std::function<bool(float)>;
    bool testAnd(glm::vec3 vec, const VecPredicate& pred);
    bool testOr(glm::vec3 vec, const VecPredicate& pred);

    using VecFunctor = std::function<float(float)>;
    glm::vec3 map(glm::vec3 vec, const VecFunctor& f);

    glm::vec3 floor(glm::vec3 vec);

    glm::vec3 abs(glm::vec3 vec);
}

#endif //VOXEL_VEC_H
