#include "chunk-manager.h"

void ChunkManager::render(OpenGLRenderer& renderer) const {
    for (auto &chunk : renderChunks) {
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
    // todo
}

void ChunkManager::updateUnloadList() {
    // todo
}

void ChunkManager::updateRebuildList() {
    for (auto &chunk : rebuildChunks) {
        chunk->markDirty();
    }
}

void ChunkManager::updateRelevantList() {
    // todo
}

void ChunkManager::updateRenderList() {
    // todo
}
