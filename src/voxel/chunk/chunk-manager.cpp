#include <utility>
#include "chunk-manager.h"

#include <iostream>

#include "src/render/renderer.h"

void ChunkManager::ChunkSlot::bind(std::shared_ptr<Chunk> c) {
    if (isBound())
        throw std::runtime_error("tried to call bind() while already bound");

    chunk = std::move(c);
    chunk->bindMeshContext(mesh);
}

void ChunkManager::ChunkSlot::unbind() {
    if (!isBound())
        throw std::runtime_error("tried to call unbind() while not bound");

    if (chunk->isLoaded())
        chunk->unload();
    chunk = nullptr;
}

ChunkManager::ChunkManager(std::shared_ptr<OpenGLRenderer> r, std::shared_ptr<WorldGen> wg)
    : renderer(std::move(r)), worldGen(std::move(wg)) {
    auto slotIter = chunkSlots.begin();

    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
        for (int y = -RENDER_DISTANCE; y <= RENDER_DISTANCE; y++) {
            for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
                const auto newChunk = std::make_shared<Chunk>(glm::vec3(x, y, z));

                loadableChunks.push_back(newChunk);

                slotIter->bind(newChunk);
                slotIter++;
            }
        }
    }
}

void ChunkManager::renderChunks() const {
    for (const ChunkPtr &chunk: visibleChunks) {
        chunk->render(renderer);
    }
}

void ChunkManager::renderChunkOutlines() const {
    for (auto &slot: chunkSlots) {
        if (!slot.isBound())
            continue;

        OpenGLRenderer::LineType lineType = OpenGLRenderer::CHUNK_OUTLINE;
        if (std::ranges::find(visibleChunks, slot.chunk) == visibleChunks.end()) {
            lineType = OpenGLRenderer::EMPTY_CHUNK_OUTLINE;
        }

        renderer->addChunkOutline(slot.chunk->getPos() * Chunk::CHUNK_SIZE, lineType);
    }
}

void ChunkManager::tick() {
    updateChunkSlots();
    updateLoadList();
    updateRenderList();
}

void ChunkManager::updateChunkSlots() {
    const glm::vec3 currPos = renderer->getCameraPos();
    const VecUtils::Vec3Discrete currChunkPos = VecUtils::floor(currPos / static_cast<float>(Chunk::CHUNK_SIZE));

    // if we didn't cross a chunk boundary, then there's nothing to do
    if (currChunkPos == lastOccupiedChunkPos)
        return;

    lastOccupiedChunkPos = currChunkPos;

    unloadFarChunks(currChunkPos);
    loadNearChunks(currChunkPos);
}

void ChunkManager::unloadFarChunks(const VecUtils::Vec3Discrete &currChunkPos) {
    for (auto &slot: chunkSlots) {
        if (!slot.isBound()) continue;

        const glm::vec3 chunkPosDist = VecUtils::abs(slot.chunk->getPos() - currChunkPos);
        const bool isOutsideRenderDistance = VecUtils::any(
            chunkPosDist,
            [](const float x) { return x > RENDER_DISTANCE + GRACE_PERIOD_WIDTH; }
        );

        if (isOutsideRenderDistance) {
            slot.unbind();
        }
    }

    erase_if(loadableChunks, [&](const ChunkPtr &chunk) {
        return VecUtils::any(
            VecUtils::abs(chunk->getPos() - currChunkPos),
            [](const float x) { return x > RENDER_DISTANCE + GRACE_PERIOD_WIDTH; }
        );
    });
}

void ChunkManager::loadNearChunks(const VecUtils::Vec3Discrete &currChunkPos) {
    constexpr size_t newChunkLoadCubeWidth = 2 * RENDER_DISTANCE + 1;
    CubeArray<bool, newChunkLoadCubeWidth> loadedChunksMap{};

    // check which positions, relative to ours, are occupied by loaded chunks
    for (auto &slot: chunkSlots) {
        if (!slot.isBound()) continue;

        const VecUtils::Vec3Discrete relPos = slot.chunk->getPos() - currChunkPos;

        // relPos components shifted so that they are non-negative
        const size_t x = relPos.x + RENDER_DISTANCE;
        const size_t y = relPos.y + RENDER_DISTANCE;
        const size_t z = relPos.z + RENDER_DISTANCE;

        // we're interested only in positions within render distance -- we don't care about chunks that are
        // still loaded only because they're in the grace period
        if (x < newChunkLoadCubeWidth && y < newChunkLoadCubeWidth && z < newChunkLoadCubeWidth) {
            loadedChunksMap[x][y][z] = true;
        }
    }

    // use the previously gathered information to load missing chunks to free slots
    auto slotIt = chunkSlots.begin();

    loadedChunksMap.forEach([&](const size_t x, const size_t y, const size_t z, const bool &isLoaded) {
        if (isLoaded) return;

        // chunk at `newChunkPos` is unloaded but should be -- we'll load it
        const glm::vec3 newChunkPos = currChunkPos + VecUtils::Vec3Discrete(x, y, z) - RENDER_DISTANCE;
        const auto newChunk = std::make_shared<Chunk>(newChunkPos);
        loadableChunks.push_back(newChunk);

        // find a free slot and bind it with the new chunk
        while (slotIt->isBound()) {
            slotIt++;
        }

        slotIt->bind(newChunk);
    });
}

void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

    // sort the chunks we will load so that the chunks closest to the camera get loaded first
    const glm::vec3 cameraPos = renderer->getCameraPos();
    std::ranges::sort(loadableChunks, [&](const ChunkPtr& a, const ChunkPtr& b) {
        const auto aDist = glm::length(cameraPos - static_cast<glm::vec3>(a->getPos() * Chunk::CHUNK_SIZE));
        const auto bDist = glm::length(cameraPos - static_cast<glm::vec3>(b->getPos() * Chunk::CHUNK_SIZE));
        return aDist < bDist;
    });

    for (const ChunkPtr &chunk: loadableChunks) {
        if (!chunk->isLoaded()) {
            chunk->generate(worldGen); // todo - this should load from disk, not always generate.
            nChunksLoaded++;
        }

        if (nChunksLoaded == MAX_CHUNKS_SERVE_PER_PRAME) {
            break;
        }
    }

    erase_if(loadableChunks, [](const ChunkPtr &chunk) { return chunk->isLoaded(); });
}

void ChunkManager::updateRenderList() {
    // clear the render list each frame BEFORE we do our tests to see what chunks should be rendered
    visibleChunks.clear();

    for (auto &slot: chunkSlots) {
        if (!slot.isBound())
            continue;
        if (!slot.chunk->shouldRender()) // early flags check, so we don't always have to do the frustum check...
            continue;
        if (!renderer->isChunkInFrustum(*slot.chunk))
            continue;

        visibleChunks.push_back(slot.chunk);
    }
}

std::optional<glm::vec3>
ChunkManager::getTargetedBlock(const std::vector<VecUtils::Vec3Discrete> &lookedAtBlocks) const {
    for (auto &block: lookedAtBlocks) {
        const ChunkPtr chunk = getOwningChunk(block);
        if (!chunk) continue;

        const VecUtils::Vec3Discrete relativeBlockPos = block - chunk->getPos() * Chunk::CHUNK_SIZE;
        const EBlockType blockType = chunk->getBlock(relativeBlockPos);

        if (blockType != BlockType_None) {
            return block;
        }
    }

    return {};
}

void ChunkManager::updateBlock(const VecUtils::Vec3Discrete &block, const EBlockType type) const {
    const ChunkPtr chunk = getOwningChunk(block);
    if (!chunk) return;

    const VecUtils::Vec3Discrete relativeBlockPos = block - chunk->getPos() * Chunk::CHUNK_SIZE;
    chunk->updateBlock(relativeBlockPos, type);
}

ChunkManager::ChunkPtr ChunkManager::getOwningChunk(const VecUtils::Vec3Discrete &block) const {
    const VecUtils::Vec3Discrete owningChunkPos =
            VecUtils::floor(static_cast<glm::vec3>(block) * (1.0f / Chunk::CHUNK_SIZE));

    const auto it = std::ranges::find_if(chunkSlots, [&](const ChunkSlot &slot) {
        return slot.isBound() && slot.chunk->isLoaded() && slot.chunk->getPos() == owningChunkPos;
    });

    return it != chunkSlots.end()
               ? it->chunk
               : nullptr;
}
