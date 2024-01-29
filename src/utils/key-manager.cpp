#include "key-manager.h"

void KeyManager::bindCallback(const EKey k, const EActivationType type, const EKeyCallback& f) {
    callbackMap[k] = {type, f};
    keyStateMap[k] = KeyState::RELEASED;
}

void KeyManager::tick(const float deltaTime) {
    for (const auto &[key, callbackInfo]: callbackMap) {
        const auto &[activationType, callback] = callbackInfo;

        if (checkKey(key, activationType)) {
            callback(deltaTime);
        }
    }
}


bool KeyManager::checkKey(const EKey key, const EActivationType type) {
    if (type == EActivationType::PRESS_ANY) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    if (type == EActivationType::RELEASE_ONCE) {
        return glfwGetKey(window, key) == GLFW_RELEASE;
    }

    if (type == EActivationType::PRESS_ONCE) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            const bool isOk = keyStateMap[key] == KeyState::RELEASED;
            keyStateMap[key] = KeyState::PRESSED;
            return isOk;
        }

        keyStateMap[key] = KeyState::RELEASED;
    }

    return false;
}
