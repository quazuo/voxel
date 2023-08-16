#include <iostream>
#include "chunk-manager.h"

#include "src/render/renderer.h"

void ChunkManager::init() {
    for (auto& slot : chunkSlots) {
        slot.init();
    }

    auto slotIter = chunkSlots.begin();

    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
        for (int y = -RENDER_DISTANCE; y <= RENDER_DISTANCE; y++) {
            for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
                const auto newChunk = std::make_shared<Chunk>(glm::vec3(x, y, z));

                loadChunks.push_back(newChunk);

                slotIter->bind(newChunk);
                slotIter++;
            }
        }
    }
}

void ChunkManager::render() const {
    renderer->startRendering();

    for (const ChunkPtr &chunk: renderChunks) {
        chunk->render(renderer);
    }

    renderOutlines();

    renderer->finishRendering();
}

void ChunkManager::renderOutlines() const {
    for (auto &slot : chunkSlots) {
        if (!slot.isBound())
            continue;

        glm::vec3 color = {1, 1, 0}; // yellow
        if (std::find(renderChunks.begin(), renderChunks.end(), slot.chunk) == renderChunks.end()) {
            color = {1, 0, 0}; // red
        }

        renderer->renderChunkOutline(slot.chunk->getPos() * Chunk::CHUNK_SIZE, color);
    }
}

void ChunkManager::tick() {
    updateChunkSlots();
    updateLoadList();
    updateUnloadList();
    updateRenderList();
}

void ChunkManager::updateChunkSlots() {
    const glm::vec3 currPos = renderer->getCameraPos();
    const VecUtils::Vec3Discrete currChunkPos = VecUtils::floor(currPos / (float) Chunk::CHUNK_SIZE);

    // if we didn't cross a chunk boundary, then there's nothing to do
    if (currChunkPos == lastOccupiedChunkPos)
        return;

    lastOccupiedChunkPos = currChunkPos;

    unloadFarChunks(currChunkPos);
    loadNearChunks(currChunkPos);
}

void ChunkManager::unloadFarChunks(VecUtils::Vec3Discrete currChunkPos) {
    for (auto &slot : chunkSlots) {
        if (!slot.isBound())
            continue;

        glm::vec3 chunkPosDist = VecUtils::abs(slot.chunk->getPos() - currChunkPos);
        bool isOutsideRenderDistance = VecUtils::testOr(
            chunkPosDist,
            [](float x) { return x > RENDER_DISTANCE + GRACE_PERIOD_WIDTH; }
        );

        if (isOutsideRenderDistance)
            slot.unbind();
    }
}

void ChunkManager::loadNearChunks(VecUtils::Vec3Discrete currChunkPos) {
    // check which positions, relative to ours, are occupied by loaded chunks
    constexpr size_t newChunkLoadCubeWidth = 2 * RENDER_DISTANCE + 1;
    SizeUtils::CubeArray<bool, newChunkLoadCubeWidth> loadedChunksMap;

    for (size_t x = 0; x < newChunkLoadCubeWidth; x++) {
        for (size_t y = 0; y < newChunkLoadCubeWidth; y++) {
            for (size_t z = 0; z < newChunkLoadCubeWidth; z++) {
                loadedChunksMap[x][y][z] = false;
            }
        }
    }

    for (auto &slot : chunkSlots) {
        if (!slot.isBound())
            continue;

        VecUtils::Vec3Discrete relPos = slot.chunk->getPos() - currChunkPos;
        size_t x = relPos.x + RENDER_DISTANCE;
        size_t y = relPos.y + RENDER_DISTANCE;
        size_t z = relPos.z + RENDER_DISTANCE;

        // we're interested only in positions within render distance -- we don't care about chunks that are
        // still loaded only because they're in the grace period
        if (x < newChunkLoadCubeWidth && y < newChunkLoadCubeWidth && z < newChunkLoadCubeWidth)
            loadedChunksMap[x][y][z] = true;
    }

    // use the previously gathered information to load missing chunks to free slots
    auto slotIt = chunkSlots.begin();

    for (size_t x = 0; x < newChunkLoadCubeWidth; x++) {
        for (size_t y = 0; y < newChunkLoadCubeWidth; y++) {
            for (size_t z = 0; z < newChunkLoadCubeWidth; z++) {
                if (loadedChunksMap[x][y][z])
                    continue;

                // chunk at `newChunkPos` is unloaded but should be -- load it
                const glm::vec3 newChunkPos = currChunkPos + VecUtils::Vec3Discrete(x, y, z) - RENDER_DISTANCE;
                const auto newChunk = std::make_shared<Chunk>(newChunkPos);
                loadChunks.push_back(newChunk);

                // find a free slot and bind it with the new chunk
                while (slotIt->isBound())
                    slotIt++;
                slotIt->bind(newChunk);
            }
        }
    }
}


void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

    for (const ChunkPtr &chunk: loadChunks) {
        if (!chunk->isLoaded()) {
            chunk->load();
            nChunksLoaded++;
        }

        if (nChunksLoaded == MAX_CHUNKS_SERVE_PER_PRAME)
            break;
    }

    erase_if(loadChunks, [](ChunkPtr &chunk) { return chunk->isLoaded(); });
}

void ChunkManager::updateUnloadList() {
    for (const ChunkPtr &chunk: unloadChunks) {
        if (chunk->isLoaded()) {
            chunk->unload();
        }
    }

    unloadChunks.clear();
}

void ChunkManager::updateRenderList() {
    // clear the render list each frame BEFORE we do our tests to see what chunks should be rendered
    renderChunks.clear();

    for (auto &slot : chunkSlots) {
        if (!slot.isBound())
            continue;
        if (!slot.chunk->shouldRender()) // early flags check, so we don't always have to do the frustum check...
            continue;
        if (!renderer->isChunkInFrustum(*slot.chunk))
            continue;
        renderChunks.push_back(slot.chunk);
    }
}
