#ifndef VOXEL_CAMERA_H
#define VOXEL_CAMERA_H

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/trigonometric.hpp"

struct Plane {
    glm::vec3 normal = {0, 0, 0};
    float distance = 0.f;
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
    float zNear = 0.1f;
    float zFar = 100.0f;

    glm::vec2 rot;
    glm::vec3 front, right, up;
    glm::vec3 pos;

    void tick(struct GLFWwindow *w, float dt);

private:
    struct GLFWwindow *window;
    float deltaTime;

    void updateRot();

    void updateVecs();

    void updatePos();

    void updateFrustum();
};

#endif //VOXEL_CAMERA_H
