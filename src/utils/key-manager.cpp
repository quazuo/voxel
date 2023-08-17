#include "key-manager.h"

void KeyManager::bindCallback(EKey k, EActivationType type, EKeyCallback f) {
    callbackMap[k] = {type, f};
    keyStateMap[k] = KeyState::RELEASED;
}

void KeyManager::tick(float deltaTime) {
    for (const auto &[key, callbackInfo]: callbackMap) {
        const auto &[activationType, callback] = callbackInfo;

        if (checkKey(key, activationType))
            callback(deltaTime);
    }
}


bool KeyManager::checkKey(EKey key, EActivationType type) {
    if (type == EActivationType::PRESS_ANY) {
        return glfwGetKey(window, key) == GLFW_PRESS;

    } else if (type == EActivationType::PRESS_ONCE) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            bool isOk = keyStateMap[key] == KeyState::RELEASED;
            keyStateMap[key] = KeyState::PRESSED;
            return isOk;

        } else {
            keyStateMap[key] = KeyState::RELEASED;
        }

    } else if (type == EActivationType::RELEASE_ONCE) {
        return glfwGetKey(window, key) == GLFW_RELEASE;
    }

    return false;
}
