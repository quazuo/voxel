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

/**
 * Class managing keyboard events, detecting them and calling certain callbacks when they occur.
 * This can safely be instantiated multiple times, handling different events across different instances.
 */
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

    /**
     * Binds a given callback to a keyboard event. Only one callback can be bound at a time,
     * so this will overwrite an earlier bound callback if there was any.
     *
     * @param k Key which on press should fire the callback.
     * @param type The way the key should be managed.
     * @param f The callback.
     */
    void bindCallback(EKey k, EActivationType type, const EKeyCallback& f);

    void tick(float deltaTime);

private:
    /**
     * Checks if a given keyboard event has occured.
     *
     * @param key Key to check.
     * @param type Type of event the caller is interested in.
     * @return Did the event occur?
     */
    bool checkKey(EKey key, EActivationType type);
};

#endif //VOXEL_KEY_MANAGER_H
