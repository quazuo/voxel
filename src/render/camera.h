#ifndef VOXEL_CAMERA_H
#define VOXEL_CAMERA_H

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/trigonometric.hpp"
#include "glm/geometric.hpp"
#include "src/utils/vec.h"

class Plane {
    glm::vec3 normal = {0, 0, 0};
    float distance = 0.f;

public:
    Plane() = default;

    Plane(glm::vec3 normalVec, glm::vec3 pointOnPlane) {
        normal = glm::normalize(normalVec); // normalize the normal vector to simplify further math
        distance = glm::dot(normal, pointOnPlane);
    }

    [[nodiscard]]
    glm::vec3 getNormal() const { return normal; }

    [[nodiscard]]
    float getDistance() const { return distance; }
};

struct Frustum {
    Plane top, bottom;
    Plane right, left;
    Plane far, near;
};

struct Camera {
    Frustum frustum;

    float aspectRatio = 4.0f / 3.0f;
    float fieldOfView = glm::radians(80.0f);
    float zNear = 1.f;
    float zFar = 500.0f;

    glm::vec2 rot;
    glm::vec3 front, right, up;
    glm::vec3 pos;

public:
    void tick(struct GLFWwindow *w, float dt);

    [[nodiscard]]
    bool isChunkInFrustum(glm::vec3 chunkPos) const;

private:
    struct GLFWwindow *window;
    float deltaTime;

    void updateRot();

    void updateVecs();

    void updatePos();

    void updateFrustum();

    [[nodiscard]]
    static bool isChunkInFrontOfPlane(glm::vec3 p, const Plane &plane);
};

#endif //VOXEL_CAMERA_H
