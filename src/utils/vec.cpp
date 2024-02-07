#include <sstream>
#include "vec.h"

int VecUtils::sum(const glm::ivec3 &v) {
    return v.x + v.y + v.z;
}

glm::vec3 VecUtils::floor(const glm::vec3 &vec) {
    static const VecFunctor f = [](const float x) { return std::floor(x); };
    return map<float>(vec, f);
}

glm::vec3 VecUtils::abs(const glm::vec3 &vec) {
    static const VecFunctor f = [](const float x) { return std::abs(x); };
    return map<float>(vec, f);
}

