#include <sstream>
#include "vec.h"

float VecUtils::distOriginToPlane(glm::vec3 planeNormal, glm::vec3 planePoint) {
    return std::abs(glm::dot(planeNormal, planePoint)) / glm::length(planeNormal);
}

std::string VecUtils::toString(glm::vec3 vec) {
    std::stringstream ss;
    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return ss.str();
}
