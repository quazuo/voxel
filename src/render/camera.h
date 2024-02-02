#ifndef VOXEL_CAMERA_H
#define VOXEL_CAMERA_H

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/matrix.hpp"
#include "glm/trigonometric.hpp"
#include "glm/geometric.hpp"
#include "src/utils/vec.h"
#include "src/utils/key-manager.h"
#include <vector>
#include <memory>

/**
 * this represents a plane, represented by its normal vector and distance to the origin (0, 0, 0).
 */
class Plane {
    glm::vec3 normal = {0, 0, 0};
    float distance = 0.f;

public:
    Plane() = default;

    Plane(const glm::vec3& normalVec, const glm::vec3& pointOnPlane)
        : normal(glm::normalize(normalVec)), distance(glm::dot(normal, pointOnPlane)) {
    }

    [[nodiscard]]
    glm::vec3 getNormal() const { return normal; }

    [[nodiscard]]
    float getDistance() const { return distance; }

    /**
     * Checks if the given chunk is at least partly in front of the plane.
     *
     * @param chunkPos Position of the chunk, given by the vertex with the lowest coordinates
     * @return Is the chunk in front of the plane?
     */
    [[nodiscard]]
    bool isChunkInFront(const VecUtils::Vec3Discrete &chunkPos) const;
};

/**
 * A frustum used to deduce what areas are visible by the camera.
 * This is used primarily for frustum culling, which excludes unseen chunks from rendering.
 */
struct Frustum {
    Plane top, bottom;
    Plane right, left;
    Plane far, near;

    /**
     * Checks if the given chunk is at least partly contained within the planes of the frustum.
     *
     * @param chunkPos Position of the chunk, given by the vertex with the lowest coordinates
     * @return Is the chunk inside the frustum?
     */
    [[nodiscard]]
    bool isChunkContained(const VecUtils::Vec3Discrete &chunkPos) const;
};

class Camera {
    Frustum frustum;

    float aspectRatio = 4.0f / 3.0f;
    float fieldOfView = 80.0f;
    float zNear = 0.1f;
    float zFar = 500.0f;

    glm::vec3 pos = {0, 0, 0};
    glm::vec2 rot = {0, 0};
    glm::vec3 front{}, right{}, up{};

    float rotationSpeed = 2.5f;
    float movementSpeed = 8.0f;
    bool isCursorLocked = true;

    static constexpr int TARGET_DISTANCE = 5;

    GLFWwindow *window;

    KeyManager keyManager;

public:
    explicit Camera(GLFWwindow *w);

    void tick(float deltaTime);

    [[nodiscard]]
    glm::vec3 getPos() const { return pos; }

    [[nodiscard]]
    glm::mat4 getViewMatrix() const;

    [[nodiscard]]
    glm::mat4 getStaticViewMatrix() const;

    [[nodiscard]]
    glm::mat4 getProjectionMatrix() const;

    /**
     * Locks or unlocks the cursor. When the cursor is locked, it's confined to the center
     * of the screen and camera rotates according to its movement. When it's unlocked, it's
     * visible and free to move around the screen; most importantly able to use the GUI.
     */
    void setIsCursorLocked(bool b);

    /**
     * Checks if the given chunk is at least partly contained within the camera's frustum.
     *
     * @param chunkPos Position of the chunk, given by the vertex with the lowest coordinates
     * @return Is the chunk inside the frustum?
     */
    [[nodiscard]]
    bool isChunkInFrustum(const VecUtils::Vec3Discrete& chunkPos) const { return frustum.isChunkContained(chunkPos); }

    /**
     * @return Vector containing positions of blocks which are under the camera's crosshair, no further from
     * the camera than the `TARGET_DISTANCE` constant (by Manhattan distance).
     */
    [[nodiscard]]
    std::vector<VecUtils::Vec3Discrete> getLookedAtBlocks() const;

    void updateRotation(float dx = 0.0f, float dy = 0.0f);

    void renderGuiSection();

private:
    /**
     * Binds keys (currently restricted to arrow keys) used to rotate the camera.
     */
    void bindRotationKeys();

    /**
     * Binds keys (currently restricted to WSAD, Space & LShift) used to move the camera.
     */
    void bindMovementKeys();

    void tickMouseMovement(float deltaTime);

    /**
     * Updates the aspect ratio to reflect the current window's dimensions.
     */
    void updateAspectRatio();

    /**
     * Updates the `front`, `right` and `up` vectors, which are used to help determine
     * what is visible to the camera.
     */
    void updateVecs();

    /**
     * Updates the planes defining the camera's view frustum.
     */
    void updateFrustum();

    /**
     * Checks if the given block is under the camera's crosshair.
     *
     * @param blockPos position of the block, given by the vertex with the lowest coordinates
     * @return is the block under the crosshair?
     */
    [[nodiscard]]
    bool isBlockLookedAt(const glm::vec3 &blockPos) const;
};

#endif //VOXEL_CAMERA_H
