#include <iostream>
#include <memory>

#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "render/renderer.h"
#include "voxel/chunk/chunk-manager.h"
#include "utils/key-manager.h"
#include "src/voxel/world-gen.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer;
    struct GLFWwindow *window = nullptr;

    std::shared_ptr<ChunkManager> chunkManager;

    std::shared_ptr<WorldGen> worldGen;

    KeyManager keyManager;

    glm::vec3 targetedBlockPos{};
    bool isTargetedBlockValid = false;

    float lastTime = 0.f;

    bool doRenderChunkOutlines = false;
    bool doRenderDebugText = true;
    bool doTick = true;

public:
    void init() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        renderer = std::make_shared<OpenGLRenderer>(1024, 768);
        window = renderer->getWindow();
        worldGen = std::make_shared<DefaultWorldGen>();
        chunkManager = std::make_shared<ChunkManager>(renderer, worldGen);
        bindKeyActions();
    }

    void startTicking() {
        while (doTick && !glfwWindowShouldClose(window)) {
            tick();
        }
    }

    void terminate() {
        renderer->terminate();
        chunkManager->terminate();
    }

    void tick() {
        // calculate this tick's delta time
        auto currentTime = (float) glfwGetTime();
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        keyManager.tick(deltaTime);
        renderer->tick(deltaTime);
        chunkManager->tick();

        // rendering
        renderer->startRendering();

        chunkManager->renderChunks();

        if (doRenderChunkOutlines)
            chunkManager->renderChunkOutlines();

        isTargetedBlockValid = chunkManager->getTargetedBlock(renderer->getLookedAtBlocks(), targetedBlockPos);
        if (isTargetedBlockValid) {
            renderer->addTargetedBlockOutline(targetedBlockPos);
        }

        renderer->renderOutlines();

        // following functions HAVE TO be called as the last thing, because while rendering overlays we clear
        // the z-buffer so that the text is on top of everything.
        // this is, of course, not desired for other rendered things.
        if (doRenderDebugText)
            renderDebugText(1 / deltaTime);
        renderer->renderHud();

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

        keyManager.bindCallback(GLFW_KEY_Q, EActivationType::PRESS_ONCE, [&](float deltaTime) {
            (void) deltaTime;
            if (isTargetedBlockValid) {
                chunkManager->updateBlock(targetedBlockPos, EBlockType::BlockType_None);
            }
        });

        keyManager.bindCallback(GLFW_KEY_F1, EActivationType::PRESS_ONCE, [&](float deltaTime) {
            (void) deltaTime;
            doRenderChunkOutlines = !doRenderChunkOutlines;
        });

        keyManager.bindCallback(GLFW_KEY_F2, EActivationType::PRESS_ONCE, [&](float deltaTime) {
            (void) deltaTime;
            doRenderDebugText = !doRenderDebugText;
        });

        keyManager.bindCallback(GLFW_KEY_ESCAPE, EActivationType::PRESS_ONCE, [&](float deltaTime) {
            (void) deltaTime;
            doTick = false;
        });
    }
};

int main() {
    VEngine engine;
    engine.init();
    engine.startTicking();
    engine.terminate();

    return 0;
}
