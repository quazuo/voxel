#include "chunk-manager.h"

void ChunkManager::render(OpenGLRenderer &renderer) const {
    for (auto &chunk: renderChunks) {
        chunk->render(renderer);
    }
}

void ChunkManager::update() {
    // todo - order?
    updateLoadList();
    updateUnloadList();
    updateRebuildList();
    updateRelevantList();
    updateRenderList();
}

void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

    for (const ChunkPtr &chunk: loadChunks) {
        if (!chunk->isLoaded()) {
            chunk->load();
            rebuildChunks.push_back(chunk);
            nChunksLoaded++;
            // forceVisibilityUpdate = true;
        }

        if (nChunksLoaded == MAX_CHUNKS_SERVE_PER_PRAME)
            break;
    }

    loadChunks.clear();
}

void ChunkManager::updateUnloadList() {
    for (const ChunkPtr &chunk: unloadChunks) {
        if (chunk->isLoaded()) {
            chunk->unload();
            // forceVisibilityUpdate = true;
        }
    }

    unloadChunks.clear();
}

void ChunkManager::updateRebuildList() {
    int nChunksRebuilt = 0;

    for (auto &chunk: rebuildChunks) {
        chunk->markDirty();
        nChunksRebuilt++;

        if (nChunksRebuilt == MAX_CHUNKS_SERVE_PER_PRAME)
            break;
    }

    // rebuildChunks.clear()
}

void ChunkManager::updateRelevantList() {
    // todo
}

void ChunkManager::updateRenderList() {
    // todo

    // clear the render list each frame BEFORE we do our tests to see what chunks should be rendered
    renderChunks.clear();

    for (ChunkPtr &chunk: relevantChunks) {
        if (!chunk->isLoaded())
            continue;
        //if (!chunk->shouldRender()) // early flags check, so we don't always have to do the frustum check...
        //    continue;

        // Check if this chunk is inside the camera frustum
        double chunkSize = Chunk::CHUNK_SIZE * Block::RENDER_SIZE;
        double chunkOffset = chunkSize - Block::RENDER_SIZE;
        Vec3 chunkCenter = chunk->getPos() + Vec3(chunkOffset, chunkOffset, chunkOffset);

        (void) chunkCenter;
        //if (renderer->chunkInFrustum(chunkCenter, chunkSize, chunkSize, chunkSize)) {
        //    renderChunks.push_back(chunk);
        //}
    }
}
