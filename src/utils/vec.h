#ifndef VOXEL_VEC_H
#define VOXEL_VEC_H

#include <string>
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

namespace VecUtils {
    float distOriginToPlane(glm::vec3 planeNormal, glm::vec3 planePoint);

    std::string toString(glm::vec3 vec);
}


#endif //VOXEL_VEC_H
