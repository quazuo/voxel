#include <memory>

#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "render/gui.h"
#include "render/renderer.h"
#include "voxel/chunk/chunk-manager.h"
#include "utils/key-manager.h"
#include "src/voxel/world-gen.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer;
    GLFWwindow *window = nullptr;

    std::shared_ptr<GuiRenderer> guiRenderer;

    std::unique_ptr<ChunkManager> chunkManager;

    std::shared_ptr<WorldGen> worldGen;

    KeyManager keyManager;

    std::optional<glm::vec3> targetedBlockPos{};

    float lastTime = 0.f;

    bool doRenderChunkOutlines = false;
    bool doRenderDebugText = true;
    bool doShowGui = false;
    bool doLockCursor = true;

public:
    void init() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        renderer = std::make_shared<OpenGLRenderer>(1024, 768);
        renderer->setIsCursorLocked(doLockCursor);

        window = renderer->getWindow();
        guiRenderer = std::make_shared<GuiRenderer>(window);
        worldGen = std::make_shared<DefaultWorldGen>();
        chunkManager = std::make_unique<ChunkManager>(renderer, worldGen);
        bindKeyActions();
    }

    void startTicking() {
        while (!glfwWindowShouldClose(window)) {
            tick();
        }
    }

    void tick() {
        // calculate this tick's delta time
        const auto currentTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        keyManager.tick(deltaTime);
        renderer->tick(deltaTime);
        chunkManager->tick();

        // rendering
        renderer->startRendering();

        renderer->renderSkybox();
        chunkManager->renderChunks();

        if (doRenderChunkOutlines)
            chunkManager->renderChunkOutlines();

        targetedBlockPos = chunkManager->getTargetedBlock(renderer->getLookedAtBlocks());
        if (targetedBlockPos) {
            renderer->addTargetedBlockOutline(*targetedBlockPos);
        }

        renderer->renderOutlines();

        // following functions HAVE TO be called as the last thing, because while rendering overlays we clear
        // the z-buffer so that the text is on top of everything.
        // this is, of course, not desired for other rendered things.
        renderer->renderHud();

        if (doShowGui) {
            guiRenderer->startRendering();
            renderer->renderGuiSection();
            guiRenderer->finishRendering();
        }

        renderer->finishRendering();
    }

    void renderDebugText(const float fps) const {
        constexpr float fontSize = 16;
        constexpr float yOffset = fontSize;
        renderer->renderText("pos: " + VecUtils::toString(renderer->getCameraPos()), 0, 0, fontSize);
        renderer->renderText("fps: " + std::to_string(fps), 0, yOffset, fontSize);
    }

    void bindKeyActions() {
        keyManager.bindWindow(window);

        keyManager.bindCallback(GLFW_KEY_Q, EActivationType::PRESS_ONCE, [&](const float deltaTime) {
            (void) deltaTime;
            if (targetedBlockPos) {
                chunkManager->updateBlock(*targetedBlockPos, EBlockType::BlockType_None);
            }
        });

        keyManager.bindCallback(GLFW_KEY_GRAVE_ACCENT, EActivationType::PRESS_ONCE, [&](const float deltaTime) {
            (void) deltaTime;
            doShowGui = !doShowGui;
        });

        keyManager.bindCallback(GLFW_KEY_F1, EActivationType::PRESS_ONCE, [&](const float deltaTime) {
            (void) deltaTime;
            doRenderChunkOutlines = !doRenderChunkOutlines;
        });

        keyManager.bindCallback(GLFW_KEY_F2, EActivationType::PRESS_ONCE, [&](const float deltaTime) {
            (void) deltaTime;
            doLockCursor = !doLockCursor;
            renderer->setIsCursorLocked(doLockCursor);
        });

        keyManager.bindCallback(GLFW_KEY_ESCAPE, EActivationType::PRESS_ONCE, [&](const float deltaTime) {
            (void) deltaTime;
            glfwSetWindowShouldClose(window, true);
        });
    }
};

int main() {
    VEngine engine;
    engine.init();
    engine.startTicking();
    return 0;
}
