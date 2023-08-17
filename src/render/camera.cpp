#include "camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/geometric.hpp"
#include "src/utils/vec.h"
#include "src/voxel/chunk/chunk.h"

void Camera::init(struct GLFWwindow *window) {
    keyManager.bindWindow(window);
    bindRotationKeys();
    bindMovementKeys();
}

void Camera::tick(float dt) {
    keyManager.tick(dt);
    updateVecs();
    updateFrustum();
}

void Camera::bindRotationKeys() {
    keyManager.bindCallback(GLFW_KEY_UP, EActivationType::PRESS_ANY, [this](float deltaTime) {
        rot.y = std::clamp(
            rot.y + deltaTime * rotationSpeed,
            -3.14f / 2,
            3.14f / 2
        );
    });

    keyManager.bindCallback(GLFW_KEY_DOWN, EActivationType::PRESS_ANY, [this](float deltaTime) {
        rot.y = std::clamp(
            rot.y - deltaTime * rotationSpeed,
            -3.14f / 2,
            3.14f / 2
        );
    });

    keyManager.bindCallback(GLFW_KEY_RIGHT, EActivationType::PRESS_ANY, [this](float deltaTime) {
        rot.x -= deltaTime * rotationSpeed;
    });

    keyManager.bindCallback(GLFW_KEY_LEFT, EActivationType::PRESS_ANY, [this](float deltaTime) {
        rot.x += deltaTime * rotationSpeed;
    });
}

void Camera::updateVecs() {
    front = {
        std::cos(rot.y) * std::sin(rot.x),
        std::sin(rot.y),
        std::cos(rot.y) * std::cos(rot.x)
    };

    right = glm::vec3(
        std::sin(rot.x - 3.14f / 2.0f),
        0,
        std::cos(rot.x - 3.14f / 2.0f)
    );

    up = glm::cross(right, front);
}

void Camera::bindMovementKeys() {
    keyManager.bindCallback(GLFW_KEY_W, EActivationType::PRESS_ANY, [this](float deltaTime) {
        pos += front * deltaTime * movementSpeed; // Move forward
    });

    keyManager.bindCallback(GLFW_KEY_S, EActivationType::PRESS_ANY, [this](float deltaTime) {
        pos -= front * deltaTime * movementSpeed; // Move backward
    });

    keyManager.bindCallback(GLFW_KEY_D, EActivationType::PRESS_ANY, [this](float deltaTime) {
        pos += right * deltaTime * movementSpeed; // Strafe right
    });

    keyManager.bindCallback(GLFW_KEY_A, EActivationType::PRESS_ANY, [this](float deltaTime) {
        pos -= right * deltaTime * movementSpeed; // Strafe left
    });

    keyManager.bindCallback(GLFW_KEY_SPACE, EActivationType::PRESS_ANY, [this](float deltaTime) {
        pos += glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly upwards
    });
}

void Camera::updateFrustum() {
    const float halfVSide = zFar * tanf(fieldOfView * 0.5f);
    const float halfHSide = halfVSide * aspectRatio;
    const glm::vec3 frontMultNear = zNear * front;
    const glm::vec3 frontMultFar = zFar * front;

    frustum.near = {front, pos + frontMultNear};
    frustum.far = {-front, pos + frontMultFar};
    frustum.right = {glm::cross(up, frontMultFar + right * halfHSide), pos};
    frustum.left = {glm::cross(frontMultFar - right * halfHSide, up), pos};
    frustum.top = {glm::cross(frontMultFar + up * halfVSide, right), pos};
    frustum.bottom = {glm::cross(right, frontMultFar - up * halfVSide), pos};
}

bool Camera::isChunkInFrustum(VecUtils::Vec3Discrete chunkPos) const {
    return isChunkInFrontOfPlane(chunkPos, frustum.near) &&
           isChunkInFrontOfPlane(chunkPos, frustum.far) &&
           isChunkInFrontOfPlane(chunkPos, frustum.top) &&
           isChunkInFrontOfPlane(chunkPos, frustum.bottom) &&
           isChunkInFrontOfPlane(chunkPos, frustum.left) &&
           isChunkInFrontOfPlane(chunkPos, frustum.right);
}

bool Camera::isChunkInFrontOfPlane(VecUtils::Vec3Discrete chunkPos, const Plane &plane) {
    const glm::vec3 chunkAbsPos = (glm::vec3) chunkPos * (float) Chunk::CHUNK_SIZE / Block::RENDER_SIZE;
    const glm::vec3 chunkMinPoint =
        chunkAbsPos - glm::vec3(Block::RENDER_SIZE / 2, Block::RENDER_SIZE / 2, Block::RENDER_SIZE / 2);
    const float chunkAbsSize = Chunk::CHUNK_SIZE * Block::RENDER_SIZE;
    const glm::vec3 chunkCenter = chunkMinPoint + glm::vec3(chunkAbsSize / 2, chunkAbsSize / 2, chunkAbsSize / 2);

    const float projectionRadius = (chunkAbsSize / 2) * std::abs(plane.getNormal().x) +
                                   (chunkAbsSize / 2) * std::abs(plane.getNormal().y) +
                                   (chunkAbsSize / 2) * std::abs(plane.getNormal().z);

    const float signedDistance = glm::dot(plane.getNormal(), chunkCenter) - plane.getDistance();

    return -projectionRadius <= signedDistance;
}
