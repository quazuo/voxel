#ifndef VOXEL_KEY_MANAGER_H
#define VOXEL_KEY_MANAGER_H

#include <functional>
#include <map>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

enum class EActivationType {
    PRESS_ANY,
    PRESS_ONCE,
    RELEASE_ONCE,
};

using EKey = int;
using EKeyCallback = std::function<void(float)>;

class KeyManager {
    struct GLFWwindow *window = nullptr;

    using KeyCallbackInfo = std::pair<EActivationType, EKeyCallback>;
    std::map<EKey, KeyCallbackInfo> callbackMap;

    enum class KeyState {
        PRESSED,
        RELEASED
    };

    std::map<EKey, KeyState> keyStateMap;

public:
    KeyManager() = default;

    explicit KeyManager(struct GLFWwindow *w) : window(w) {}

    void bindWindow(struct GLFWwindow *w) { window = w; }

    void bindCallback(EKey k, EActivationType type, EKeyCallback f);

    void tick(float deltaTime);

private:
    bool checkKey(EKey key, EActivationType type);
};

#endif //VOXEL_KEY_MANAGER_H
