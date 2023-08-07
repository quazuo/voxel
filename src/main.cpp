#include <iostream>
#include <memory>

#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "render/renderer.h"
#include "src/voxel/chunk/chunk.h"
#include "src/voxel/chunk/chunk-manager.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer;
    std::shared_ptr<ChunkManager> chunkManager;

    Chunk chunk; // todo - placeholder just to check if one chunk renders

    int renderedFrames = 0;
    float lastTime = 0.f;

public:
    VEngine() {
        renderer = std::make_shared<OpenGLRenderer>();
        chunkManager = std::make_shared<ChunkManager>();
    }

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

        renderedFrames++;
        if (currentTime - lastTime >= 1.0f) {
            std::cout << 1000.0 / (double) renderedFrames << " ms/frame\n";
            renderedFrames = 0;
        }

        lastTime = currentTime;

        renderer->tick(deltaTime);

        renderer->startRendering();
        chunkManager->render(*renderer);
        chunk.render(*renderer);
        renderer->finishRendering();
    }
};

int main() {
    VEngine engine;
    engine.init();
    engine.startTicking();

    return 0;
}
