#include <iostream>
#include <memory>

#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "render/renderer.h"
#include "src/voxel/chunk/chunk-manager.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer = std::make_shared<OpenGLRenderer>();
    std::shared_ptr<ChunkManager> chunkManager = std::make_shared<ChunkManager>(renderer);

    float lastTime = 0.f;

public:
    void init() {
        renderer->init();
    }

    [[noreturn]]
    void startTicking() {
        while (true) {
            tick();
        }
    }

    void tick() {
        // Measure speed
        auto currentTime = (float) glfwGetTime();
        const float deltaTime = currentTime - lastTime;
        std::cout << "frametime: " << deltaTime << "s, fps: " << 1 / deltaTime << "\n";
        lastTime = currentTime;

        renderer->tick(deltaTime);
        chunkManager->render();
    }
};

int main() {
    VEngine engine;
    engine.init();
    engine.startTicking();

    return 0;
}
