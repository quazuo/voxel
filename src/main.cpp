#include <iostream>
#include <memory>

#include "render/renderer.h"
#include "src/voxel/chunk/chunk.h"
#include "src/voxel/chunk/chunk-manager.h"

class VEngine {
    std::shared_ptr<OpenGLRenderer> renderer;
    std::shared_ptr<ChunkManager> chunkManager;

    Chunk chunk; // todo - placeholder just to check if one chunk renders

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
