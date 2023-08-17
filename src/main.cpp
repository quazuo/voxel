#include <iostream>
#include <memory>

#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "render/renderer.h"
#include "src/voxel/chunk/chunk-manager.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer = std::make_shared<OpenGLRenderer>();
    struct GLFWwindow *window = nullptr;

    std::shared_ptr<ChunkManager> chunkManager = std::make_shared<ChunkManager>(renderer);

    KeyManager keyManager;

    float lastTime = 0.f;

    bool doRenderChunkOutlines = true;
    bool doRenderDebugText = true;

public:
    void init() {
        window = renderer->init();
        chunkManager->init();
        bindKeyActions();
    }

    [[noreturn]]
    void startTicking() {
        while (true) {
            tick();
        }
    }

    void tick() {
        // calculate this tick's delta time
        auto currentTime = (float) glfwGetTime();
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        keyManager.tick(deltaTime);
        renderer->tick(deltaTime);
        chunkManager->tick();

        renderer->startRendering();
        chunkManager->renderChunks();
        if (doRenderChunkOutlines)
            chunkManager->renderChunkOutlines();
        if (doRenderDebugText)
            renderDebugText(1 / deltaTime);
        renderer->finishRendering();
    }

    void renderDebugText(float fps) {
        constexpr float fontSize = 16;
        constexpr float yOffset = fontSize;
        renderer->renderText("pos: " + VecUtils::toString(renderer->getCameraPos()), 0, 0, fontSize);
        renderer->renderText("fps: " + std::to_string(fps), 0, yOffset, fontSize);
    }

    void bindKeyActions() {
        keyManager.bindWindow(window);

        keyManager.bindCallback(GLFW_KEY_F1, EActivationType::PRESS_ONCE, [this](float deltaTime) {
            (void) deltaTime;
            doRenderChunkOutlines = !doRenderChunkOutlines;
        });

        keyManager.bindCallback(GLFW_KEY_F2, EActivationType::PRESS_ONCE, [this](float deltaTime) {
            (void) deltaTime;
            doRenderDebugText = !doRenderDebugText;
        });
    }
};

int main() {
    VEngine engine;
    engine.init();
    engine.startTicking();

    return 0;
}
