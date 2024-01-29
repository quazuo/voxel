#include <sstream>
#include "vec.h"

int VecUtils::sum(const Vec3Discrete &v) {
    return v.x + v.y + v.z;
}

std::string VecUtils::toString(const glm::vec3 &vec) {
    std::stringstream ss;
    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return ss.str();
}

bool VecUtils::all(const glm::vec3 &vec, const VecPredicate& pred) {
    return pred(vec.x) && pred(vec.y) && pred(vec.z);
}

bool VecUtils::any(const glm::vec3 &vec, const VecPredicate& pred) {
    return pred(vec.x) || pred(vec.y) || pred(vec.z);
}

glm::vec3 VecUtils::map(glm::vec3 vec, const VecFunctor& f) {
    vec.x = f(vec.x);
    vec.y = f(vec.y);
    vec.z = f(vec.z);
    return vec;
}

glm::vec3 VecUtils::floor(const glm::vec3 &vec) {
    static const VecFunctor f = [](const float x) { return std::floor(x); };
    return map(vec, f);
}

glm::vec3 VecUtils::abs(const glm::vec3 &vec) {
    static const VecFunctor f = [](const float x) { return std::abs(x); };
    return map(vec, f);
}

