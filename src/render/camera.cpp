#include "camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/geometric.hpp"
#include "src/utils/vec.h"

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
        pos += glm::vec3(front * deltaTime * movementSpeed); // Move forward
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        pos -= glm::vec3(front * deltaTime * movementSpeed); // Move backward
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        pos += glm::vec3(right * deltaTime * movementSpeed); // Strafe right
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        pos -= glm::vec3(right * deltaTime * movementSpeed); // Strafe left
    }
}

void Camera::updateFrustum() {
    const float halfVSide = zFar * tanf(fieldOfView * 0.5f);
    const float halfHSide = halfVSide * aspectRatio;
    const glm::vec3 frontMultFar = zFar * front;

    frustum.near.normal = front;
    frustum.near.distance = zNear;

    frustum.far.normal = -front;
    frustum.far.distance = zFar;

    frustum.right.normal = glm::cross(up, frontMultFar + right * halfHSide);
    frustum.right.distance = VecUtils::distOriginToPlane(frustum.right.normal, pos);

    frustum.left.normal = glm::cross(frontMultFar - right * halfHSide, up);
    frustum.left.distance = VecUtils::distOriginToPlane(frustum.left.normal, pos);

    frustum.top.normal = glm::cross(frontMultFar + up * halfVSide, right);
    frustum.top.distance = VecUtils::distOriginToPlane(frustum.top.normal, pos);

    frustum.bottom.normal = glm::cross(right, frontMultFar - up * halfVSide);
    frustum.bottom.distance = VecUtils::distOriginToPlane(frustum.bottom.normal, pos);
}
