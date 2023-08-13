#include "chunk-manager.h"

#include "src/render/renderer.h"

void ChunkManager::render() const {
    renderer->startRendering();

    for (const ChunkPtr &chunk: renderChunks) {
        chunk->render(renderer);
    }

    renderOutlines();

    renderer->finishRendering();
}

void ChunkManager::renderOutlines() const {
    const float chunkAbsSize = (float) Chunk::CHUNK_SIZE / Block::RENDER_SIZE;

    for (const ChunkPtr &chunk: relevantChunks) {
        glm::vec3 color = {1, 1, 0}; // yellow
        if (std::find(renderChunks.begin(), renderChunks.end(), chunk) == renderChunks.end()) {
            color = {1, 0, 0}; // red
        }

        renderer->renderChunkOutline(chunk->getPos() * chunkAbsSize, color);
    }
}

void ChunkManager::update() {
    // todo - order?
    updateLoadList();
    updateUnloadList();
    updateRelevantList();
    updateRenderList();
}

void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

    for (const ChunkPtr &chunk: loadChunks) {
        if (!chunk->isLoaded()) {
            chunk->load();
            relevantChunks.push_back(chunk);
            nChunksLoaded++;
        }

        if (nChunksLoaded == MAX_CHUNKS_SERVE_PER_PRAME)
            break;
    }

    // loadChunks.clear();
}

void ChunkManager::updateUnloadList() {
    for (const ChunkPtr &chunk: unloadChunks) {
        if (chunk->isLoaded()) {
            chunk->unload();
        }
    }

    unloadChunks.clear();
}

void ChunkManager::updateRelevantList() {
    // todo
}

void ChunkManager::updateRenderList() {
    // clear the render list each frame BEFORE we do our tests to see what chunks should be rendered
    renderChunks.clear();

    for (const ChunkPtr &chunk: relevantChunks) {
        if (!chunk->shouldRender()) // early flags check, so we don't always have to do the frustum check...
            continue;
        if (!renderer->isChunkInFrustum(*chunk))
            continue;
        renderChunks.push_back(chunk);
    }
}
