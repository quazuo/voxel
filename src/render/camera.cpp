#include "camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "glm/geometric.hpp"
#include "src/utils/vec.h"
#include "src/voxel/chunk/chunk.h"

void Camera::init(struct GLFWwindow *w) {
    window = w;
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
    keyManager.bindCallback(GLFW_KEY_UP, EActivationType::PRESS_ANY, [&](float deltaTime) {
        updateRotation(0.0f, deltaTime * rotationSpeed);
    });

    keyManager.bindCallback(GLFW_KEY_DOWN, EActivationType::PRESS_ANY, [&](float deltaTime) {
        updateRotation(0.0f, -deltaTime * rotationSpeed);
    });

    keyManager.bindCallback(GLFW_KEY_RIGHT, EActivationType::PRESS_ANY, [&](float deltaTime) {
        rot.x -= deltaTime * rotationSpeed;
    });

    keyManager.bindCallback(GLFW_KEY_LEFT, EActivationType::PRESS_ANY, [&](float deltaTime) {
        rot.x += deltaTime * rotationSpeed;
    });
}

void Camera::updateRotation(float dx, float dy) {
    rot.x += dx;
    rot.y = std::clamp(
        rot.y + dy,
        -3.14f / 2,
        3.14f / 2
    );
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
    keyManager.bindCallback(GLFW_KEY_W, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos += front * deltaTime * movementSpeed; // Move forward
    });

    keyManager.bindCallback(GLFW_KEY_S, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos -= front * deltaTime * movementSpeed; // Move backward
    });

    keyManager.bindCallback(GLFW_KEY_D, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos += right * deltaTime * movementSpeed; // Strafe right
    });

    keyManager.bindCallback(GLFW_KEY_A, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos -= right * deltaTime * movementSpeed; // Strafe left
    });

    keyManager.bindCallback(GLFW_KEY_SPACE, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos += glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly upwards
    });

    keyManager.bindCallback(GLFW_KEY_LEFT_SHIFT, EActivationType::PRESS_ANY, [&](float deltaTime) {
        pos -= glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly downwards
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

bool Camera::isChunkInFrustum(const VecUtils::Vec3Discrete chunkPos) const {
    return isChunkInFrontOfPlane(chunkPos, frustum.near) &&
           isChunkInFrontOfPlane(chunkPos, frustum.far) &&
           isChunkInFrontOfPlane(chunkPos, frustum.top) &&
           isChunkInFrontOfPlane(chunkPos, frustum.bottom) &&
           isChunkInFrontOfPlane(chunkPos, frustum.left) &&
           isChunkInFrontOfPlane(chunkPos, frustum.right);
}

bool Camera::isChunkInFrontOfPlane(const VecUtils::Vec3Discrete chunkPos, const Plane &plane) {
    const glm::vec3 chunkAbsPos = (glm::vec3) chunkPos * (float) Chunk::CHUNK_SIZE / Block::RENDER_SIZE;
    const glm::vec3 chunkMinPoint = chunkAbsPos - glm::vec3(Block::RENDER_SIZE / 2);
    const float chunkAbsSize = Chunk::CHUNK_SIZE * Block::RENDER_SIZE;
    const glm::vec3 chunkCenter = chunkMinPoint + glm::vec3(chunkAbsSize / 2);

    const float projectionRadius = (chunkAbsSize / 2) * std::abs(plane.getNormal().x) +
                                   (chunkAbsSize / 2) * std::abs(plane.getNormal().y) +
                                   (chunkAbsSize / 2) * std::abs(plane.getNormal().z);

    const float signedDistance = glm::dot(plane.getNormal(), chunkCenter) - plane.getDistance();

    return -projectionRadius <= signedDistance;
}

std::vector<VecUtils::Vec3Discrete> Camera::getLookedAtBlocks() const {
    std::vector<VecUtils::Vec3Discrete> result;
    constexpr size_t size = 2 * TARGET_DISTANCE + 1;

    for (size_t x = 0; x < size; x++) {
        for (size_t y = 0; y < size; y++) {
            for (size_t z = 0; z < size; z++) {
                VecUtils::Vec3Discrete blockPos = VecUtils::floor(pos + glm::vec3(x, y, z) - (float) TARGET_DISTANCE);

                if (isBlockLookedAt(blockPos))
                    result.push_back(blockPos);
            }
        }
    }

    std::sort(result.begin(), result.end(), [&](glm::vec3 a, glm::vec3 b) {
        return glm::length(a - pos) < glm::length(b - pos);
    });

    return result;
}

bool Camera::isBlockLookedAt(const glm::vec3 blockPos) const {
    float tmin = -INFINITY, tmax = INFINITY;
    const glm::vec3 blockMin = blockPos - Block::RENDER_SIZE / 2;
    const glm::vec3 blockMax = blockMin + Block::RENDER_SIZE;

    if (front.x == 0 && front.y == 0) {
        return blockMin.x <= pos.x && pos.x <= blockMax.x &&
               blockMin.y <= pos.y && pos.y <= blockMax.y;
    }

    if (front.x == 0 && front.z == 0) {
        return blockMin.x <= pos.x && pos.x <= blockMax.x &&
               blockMin.z <= pos.z && pos.z <= blockMax.z;
    }

    if (front.y == 0 && front.z == 0) {
        return blockMin.y <= pos.y && pos.y <= blockMax.y &&
               blockMin.z <= pos.z && pos.z <= blockMax.z;
    }

    if (front.x != 0) {
        float tx1 = (blockMin.x - pos.x) / front.x;
        float tx2 = (blockMax.x - pos.x) / front.x;

        tmin = std::max(tmin, std::min(tx1, tx2));
        tmax = std::min(tmax, std::max(tx1, tx2));
    }

    if (front.y != 0) {
        float ty1 = (blockMin.y - pos.y) / front.y;
        float ty2 = (blockMax.y - pos.y) / front.y;

        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));
    }

    if (front.z != 0) {
        float tz1 = (blockMin.z - pos.z) / front.z;
        float tz2 = (blockMax.z - pos.z) / front.z;

        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));
    }

    return tmin <= tmax;
}