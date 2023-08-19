#ifndef VOXEL_CAMERA_H
#define VOXEL_CAMERA_H

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/trigonometric.hpp"
#include "glm/geometric.hpp"
#include "src/utils/vec.h"
#include "src/utils/key-manager.h"

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

class Camera {
    friend class OpenGLRenderer;

    Frustum frustum;

    float aspectRatio = 4.0f / 3.0f;
    float fieldOfView = glm::radians(80.0f);
    float zNear = 0.1f;
    float zFar = 500.0f;

    glm::vec2 rot = {0, 0};
    glm::vec3 front, right, up;
    glm::vec3 pos = {0, 10, 0};

    float rotationSpeed = 2.5f;
    float movementSpeed = 8.0f;

    static constexpr size_t TARGET_DISTANCE = 5;

public:
    void init(struct GLFWwindow *w);

    void tick(float dt);

    [[nodiscard]]
    bool isChunkInFrustum(VecUtils::Vec3Discrete chunkPos) const;

    [[nodiscard]]
    std::vector<VecUtils::Vec3Discrete> getLookedAtBlocks() const;

    void updateRotation(float dx = 0.0f, float dy = 0.0f);

private:
    struct GLFWwindow *window;

    KeyManager keyManager;

    void bindRotationKeys();

    void bindMovementKeys();

    void updateVecs();

    void updateFrustum();

    [[nodiscard]]
    static bool isChunkInFrontOfPlane(VecUtils::Vec3Discrete chunkPos, const Plane &plane);

    [[nodiscard]]
    bool isBlockLookedAt(glm::vec3 blockPos) const;
};

#endif //VOXEL_CAMERA_H
