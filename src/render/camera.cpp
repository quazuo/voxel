#include "camera.h"

#include <algorithm>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/geometric.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "src/voxel/chunk/chunk.h"
#include "gui.h"

bool Plane::isChunkInFront(const glm::ivec3 &chunkPos) const {
    const glm::vec3 chunkMinPoint = static_cast<glm::vec3>(chunkPos) * static_cast<float>(Chunk::CHUNK_SIZE);
    constexpr float chunkAbsSize = Chunk::CHUNK_SIZE;
    const glm::vec3 chunkCenter = chunkMinPoint + glm::vec3(chunkAbsSize / 2);

    const float projectionRadius = (chunkAbsSize / 2) * std::abs(normal.x) +
                                   (chunkAbsSize / 2) * std::abs(normal.y) +
                                   (chunkAbsSize / 2) * std::abs(normal.z);

    const float signedDistance = glm::dot(normal, chunkCenter) - distance;

    return -projectionRadius <= signedDistance;
}

bool Frustum::isChunkContained(const glm::ivec3 &chunkPos) const {
    return near.isChunkInFront(chunkPos) &&
           far.isChunkInFront(chunkPos) &&
           top.isChunkInFront(chunkPos) &&
           bottom.isChunkInFront(chunkPos) &&
           left.isChunkInFront(chunkPos) &&
           right.isChunkInFront(chunkPos);
}

Camera::Camera(GLFWwindow *w) : window(w) {
    keyManager.bindWindow(window);
    bindRotationKeys();
    bindMovementKeys();
}

void Camera::tick(const float deltaTime) {
    keyManager.tick(deltaTime);
    tickMouseMovement(deltaTime);
    updateAspectRatio();
    updateVecs();
    updateFrustum();
}

void Camera::bindRotationKeys() {
    keyManager.bindCallback(GLFW_KEY_UP, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        updateRotation(0.0f, deltaTime * rotationSpeed);
    });

    keyManager.bindCallback(GLFW_KEY_DOWN, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        updateRotation(0.0f, -deltaTime * rotationSpeed);
    });

    keyManager.bindCallback(GLFW_KEY_RIGHT, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        rot.x -= deltaTime * rotationSpeed;
    });

    keyManager.bindCallback(GLFW_KEY_LEFT, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        rot.x += deltaTime * rotationSpeed;
    });
}

void Camera::updateRotation(const float dx, const float dy) {
    rot.x += dx;
    rot.y = std::clamp(
        rot.y + dy,
        -3.14f / 2,
        3.14f / 2
    );
}

void Camera::renderGuiSection() {
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    constexpr auto sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::CollapsingHeader("Camera ", sectionFlags)) {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Rotation: (%.2f, %.2f)", rot.x, rot.y);
        ImGui::Text("Axes:");

        if (ImGui::BeginChild("Axes", ImVec2(50, 50))) {
            drawList->AddRectFilled(
                ImGui::GetWindowPos(),
                ImGui::GetWindowPos() + ImVec2(50, 50),
                IM_COL32(0, 0, 0, 255)
            );

            const ImVec2 offset = ImGui::GetWindowPos() + ImVec2(25, 25);
            constexpr float scale = 20;
            const glm::mat4 view = getStaticViewMatrix();
            constexpr auto projectionX = glm::vec3(1, 0, 0);
            constexpr auto projectionY = glm::vec3(0, 1, 0);

            const glm::vec3 x = view * glm::vec4(1, 0, 0, 0);
            const float tx1 = scale * glm::dot(projectionX, x);
            const float tx2 = scale * glm::dot(projectionY, x);
            drawList->AddLine(offset, offset + ImVec2(tx1, -tx2), IM_COL32(255, 0, 0, 255));

            const glm::vec3 y = view * glm::vec4(0, 1, 0, 0);
            const float ty1 = scale * glm::dot(projectionX, y);
            const float ty2 = scale * glm::dot(projectionY, y);
            drawList->AddLine(offset, offset + ImVec2(ty1, -ty2), IM_COL32(0, 255, 0, 255));

            const glm::vec3 z = view * glm::vec4(0, 0, 1, 0);
            const float tz1 = scale * glm::dot(projectionX, z);
            const float tz2 = scale * glm::dot(projectionY, z);
            drawList->AddLine(offset, offset + ImVec2(tz1, -tz2), IM_COL32(0, 0, 255, 255));
        }
        ImGui::EndChild();

        ImGui::SliderFloat("Field of view", &fieldOfView, 20.0f, 160.0f, "%.0f");
        ImGui::DragFloat("Rotation speed", &rotationSpeed, 1.0f, 0.0f, FLT_MAX, "%.0f");
        ImGui::DragFloat("Movement speed", &movementSpeed, 1.0f, 0.0f, FLT_MAX, "%.0f");
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

void Camera::bindMovementKeys() {
    keyManager.bindCallback(GLFW_KEY_W, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos += front * deltaTime * movementSpeed; // Move forward
    });

    keyManager.bindCallback(GLFW_KEY_S, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos -= front * deltaTime * movementSpeed; // Move backward
    });

    keyManager.bindCallback(GLFW_KEY_D, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos += right * deltaTime * movementSpeed; // Strafe right
    });

    keyManager.bindCallback(GLFW_KEY_A, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos -= right * deltaTime * movementSpeed; // Strafe left
    });

    keyManager.bindCallback(GLFW_KEY_SPACE, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos += glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly upwards
    });

    keyManager.bindCallback(GLFW_KEY_LEFT_SHIFT, EActivationType::PRESS_ANY, [&](const float deltaTime) {
        pos -= glm::vec3(0, 1, 0) * deltaTime * movementSpeed; // Fly downwards
    });
}

void Camera::setIsCursorLocked(const bool b) {
    isCursorLocked = b;

    if (isCursorLocked) {
        glm::vec<2, int> windowSize{};
        glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

        glfwSetCursorPos(
            window,
            static_cast<double>(windowSize.x) / 2,
            static_cast<double>(windowSize.y) / 2
        );
    }
}

void Camera::tickMouseMovement(const float deltaTime) {
    if (!isCursorLocked) return;

    glm::vec<2, double> cursorPos{};
    glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);

    glm::vec<2, int> windowSize{};
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

    const float mouseSpeed = 0.002f * rotationSpeed;
    updateRotation(
        mouseSpeed * (static_cast<float>(windowSize.x) / 2.f - static_cast<float>(cursorPos.x)),
        mouseSpeed * (static_cast<float>(windowSize.y) / 2.f - static_cast<float>(cursorPos.y))
    );
    glfwSetCursorPos(
        window,
        static_cast<double>(windowSize.x) / 2,
        static_cast<double>(windowSize.y) / 2
    );
}

void Camera::updateAspectRatio() {
    glm::vec<2, int> windowSize{};
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);
    aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
}

void Camera::updateFrustum() {
    const float halfVSide = zFar * tanf(glm::radians(fieldOfView) * 0.5f);
    const float halfHSide = halfVSide * aspectRatio;
    const glm::vec3 frontMultNear = zNear * front;
    const glm::vec3 frontMultFar = zFar * front;

    frustum.near = {
        front,
        pos + frontMultNear
    };
    frustum.far = {
        -front,
        pos + frontMultFar
    };
    frustum.right = {
        glm::cross(up, frontMultFar + right * halfHSide),
        pos
    };
    frustum.left = {
        glm::cross(frontMultFar - right * halfHSide, up),
        pos
    };
    frustum.top = {
        glm::cross(frontMultFar + up * halfVSide, right),
        pos
    };
    frustum.bottom = {
        glm::cross(right, frontMultFar - up * halfVSide),
        pos
    };
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(pos, pos + front, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getStaticViewMatrix() const {
    return glm::lookAt(glm::vec3(0), front, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, zNear, zFar);
}

std::vector<glm::ivec3> Camera::getLookedAtBlocks() const {
    std::vector<glm::vec3> normalsToCheck;

    for (const auto face: blockFaces) {
        const glm::vec3 normal = getNormalFromFace(face);

        if (glm::dot(normal, front) > 0) {
            normalsToCheck.push_back(normal);
        }
    }

    std::vector<glm::ivec3> result;
    glm::vec3 currPos = VecUtils::floor(pos);
    constexpr size_t MAX_TARGET_DISTANCE = 10;

    for (size_t i = 0; i < MAX_TARGET_DISTANCE; i++) {
        result.emplace_back(currPos);

        for (const auto &normal: normalsToCheck) {
            if (isBlockLookedAt(currPos + normal)) {
                currPos += normal;
                break;
            }
        }
    }

    return result;
}

bool Camera::isBlockLookedAt(const glm::ivec3 &blockPos) const {
    float tmin = -INFINITY, tmax = INFINITY;
    const glm::vec3 blockMin = blockPos;
    const glm::vec3 blockMax = blockMin + Block::RENDER_SIZE;

    if (glm::dot(glm::vec3(blockPos) - pos, front) < 0) {
        return false;
    }

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
        const float tx1 = (blockMin.x - pos.x) / front.x;
        const float tx2 = (blockMax.x - pos.x) / front.x;

        tmin = std::max(tmin, std::min(tx1, tx2));
        tmax = std::min(tmax, std::max(tx1, tx2));
    }

    if (front.y != 0) {
        const float ty1 = (blockMin.y - pos.y) / front.y;
        const float ty2 = (blockMax.y - pos.y) / front.y;

        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));
    }

    if (front.z != 0) {
        const float tz1 = (blockMin.z - pos.z) / front.z;
        const float tz2 = (blockMax.z - pos.z) / front.z;

        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));
    }

    return tmin <= tmax;
}