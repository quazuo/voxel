#include <sstream>
#include "vec.h"

std::string VecUtils::toString(glm::vec3 vec) {
    std::stringstream ss;
    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return ss.str();
}
