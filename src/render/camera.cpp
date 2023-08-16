#include "camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/geometric.hpp"
#include "src/utils/vec.h"
#include "src/voxel/chunk/chunk.h"

void Camera::tick(struct GLFWwindow *w, float dt) {
    window = w;
    deltaTime = dt;
    updateRot();
    updateVecs();
    updatePos();
    updateFrustum();
}

void Camera::updateRot() {
    const float rotationSpeed = 2.5;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rot.y = std::clamp(
            rot.y + deltaTime * rotationSpeed,
            -3.14f / 2,
            3.14f / 2
        );
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rot.y = std::clamp(
            rot.y - deltaTime * rotationSpeed,
            -3.14f / 2,
            3.14f / 2
        );
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rot.x -= deltaTime * rotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rot.x += deltaTime * rotationSpeed;
    }
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

void Camera::updatePos() {
    const float movementSpeed = 8.0;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        pos += front * deltaTime * movementSpeed; // Move forward
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        pos -= front * deltaTime * movementSpeed; // Move backward
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        pos += right * deltaTime * movementSpeed; // Strafe right
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        pos -= right * deltaTime * movementSpeed; // Strafe left
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        pos += glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly upwards
    }
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

bool Camera::isChunkInFrustum(glm::vec3 chunkPos) const {
    return isChunkInFrontOfPlane(chunkPos, frustum.near) &&
           isChunkInFrontOfPlane(chunkPos, frustum.far) &&
           isChunkInFrontOfPlane(chunkPos, frustum.top) &&
           isChunkInFrontOfPlane(chunkPos, frustum.bottom) &&
           isChunkInFrontOfPlane(chunkPos, frustum.left) &&
           isChunkInFrontOfPlane(chunkPos, frustum.right);
}

bool Camera::isChunkInFrontOfPlane(glm::vec3 chunkPos, const Plane &plane) {
    const glm::vec3 chunkAbsPos = chunkPos * (float) Chunk::CHUNK_SIZE / Block::RENDER_SIZE;
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
