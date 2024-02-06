#include <utility>

#include "chunk-manager.h"
#include "src/render/renderer.h"
#include "src/render/gui.h"

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
    const size_t visibleAreaWidth = 2 * renderDistance + gracePeriodWidth + 1;
    chunkSlots = std::vector<ChunkSlot>(SizeUtils::pow(visibleAreaWidth, 3));

    auto slotIter = chunkSlots.begin();

    for (int x = -renderDistance; x <= renderDistance; x++) {
        for (int y = -renderDistance; y <= renderDistance; y++) {
            for (int z = -renderDistance; z <= renderDistance; z++) {
                const auto newChunk = std::make_shared<Chunk>(glm::ivec3(x, y, z));

                loadableChunks.push_back(newChunk);

                slotIter->bind(newChunk);
                slotIter++;
            }
        }
    }

    sortLoadList();
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

void ChunkManager::renderGuiSection() {
    constexpr auto sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::CollapsingHeader("ChunkManager ", sectionFlags)) {
        ImGui::Text("Render distance: %d ", renderDistance);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { setRenderDistance(std::max(renderDistance - 1, 1)); }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { setRenderDistance(renderDistance + 1); }
    }
}

void ChunkManager::updateChunkSlots() {
    const glm::vec3 currPos = renderer->getCameraPos();
    const glm::ivec3 currChunkPos = VecUtils::floor(currPos / static_cast<float>(Chunk::CHUNK_SIZE));

    // if we didn't cross a chunk boundary, then there's nothing to do
    if (currChunkPos == lastOccupiedChunkPos)
        return;

    lastOccupiedChunkPos = currChunkPos;

    unloadFarChunks();
    loadNearChunks();
}

void ChunkManager::unloadFarChunks() {
    for (auto &slot: chunkSlots) {
        if (!slot.isBound()) continue;

        const glm::ivec3 chunkPosDist = VecUtils::abs(slot.chunk->getPos() - lastOccupiedChunkPos);
        const bool isOutsideRenderDistance = VecUtils::any(
            chunkPosDist,
            [&](const float x) { return x > static_cast<float>(renderDistance + gracePeriodWidth); }
        );

        if (isOutsideRenderDistance) {
            slot.unbind();
        }
    }

    erase_if(loadableChunks, [&](const ChunkPtr &chunk) {
        return VecUtils::any(
            VecUtils::abs(chunk->getPos() - lastOccupiedChunkPos),
            [&](const float x) { return x > static_cast<float>(renderDistance + gracePeriodWidth); }
        );
    });
}

void ChunkManager::loadNearChunks() {
    const size_t newChunkLoadCubeWidth = 2 * renderDistance + 1;
    CubeVector<uint8_t> loadedChunksMap{newChunkLoadCubeWidth}; // uint8_t instead of bool because vector<bool> sucks

    // check which positions, relative to ours, are occupied by loaded chunks
    for (auto &slot: chunkSlots) {
        if (!slot.isBound()) continue;

        const glm::ivec3 relPos = slot.chunk->getPos() - lastOccupiedChunkPos;

        // relPos components shifted so that they are non-negative
        const size_t x = relPos.x + renderDistance;
        const size_t y = relPos.y + renderDistance;
        const size_t z = relPos.z + renderDistance;

        // we're interested only in positions within render distance -- we don't care about chunks that are
        // still loaded only because they're in the grace period
        if (x < newChunkLoadCubeWidth && y < newChunkLoadCubeWidth && z < newChunkLoadCubeWidth) {
            loadedChunksMap[x][y][z] = true;
        }
    }

    // use the previously gathered information to load missing chunks to free slots
    auto slotIt = chunkSlots.begin();

    loadedChunksMap.forEach([&](const size_t x, const size_t y, const size_t z, const uint8_t &isLoaded) {
        if (isLoaded) return;

        // chunk at `newChunkPos` is unloaded but should be -- we'll load it
        const glm::ivec3 newChunkPos = lastOccupiedChunkPos + glm::ivec3(x, y, z) - renderDistance;
        const auto newChunk = std::make_shared<Chunk>(newChunkPos);
        loadableChunks.push_back(newChunk);

        // find a free slot and bind it with the new chunk
        while (slotIt->isBound()) {
            slotIt++;
        }

        slotIt->bind(newChunk);
    });

    sortLoadList();
}

void ChunkManager::sortLoadList() {
    const glm::vec3 cameraPos = renderer->getCameraPos();
    std::ranges::sort(loadableChunks, [&](const ChunkPtr &a, const ChunkPtr &b) {
        const auto aDist = glm::length(cameraPos - static_cast<glm::vec3>(a->getPos() * Chunk::CHUNK_SIZE));
        const auto bDist = glm::length(cameraPos - static_cast<glm::vec3>(b->getPos() * Chunk::CHUNK_SIZE));
        return aDist < bDist;
    });
}

void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

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

std::optional<glm::ivec3>
ChunkManager::getTargetedBlock(const std::vector<glm::ivec3> &lookedAtBlocks) const {
    for (auto &block: lookedAtBlocks) {
        const ChunkPtr chunk = getOwningChunk(block);
        if (!chunk) continue;

        const glm::ivec3 relativeBlockPos = block - chunk->getPos() * Chunk::CHUNK_SIZE;
        const EBlockType blockType = chunk->getBlock(relativeBlockPos);

        if (blockType != BlockType_None) {
            return block;
        }
    }

    return {};
}

void ChunkManager::updateBlock(const glm::ivec3 &block, const EBlockType type) const {
    const ChunkPtr chunk = getOwningChunk(block);
    if (!chunk) return;

    const glm::ivec3 relativeBlockPos = block - chunk->getPos() * Chunk::CHUNK_SIZE;
    chunk->updateBlock(relativeBlockPos, type);
}

void ChunkManager::setRenderDistance(const int newRenderDistance) {
    renderDistance = newRenderDistance;
    const size_t visibleAreaWidth = 2 * renderDistance + gracePeriodWidth + 1;
    chunkSlots = std::vector<ChunkSlot>(SizeUtils::pow(visibleAreaWidth, 3));
    loadNearChunks();

    // todo - copy already loaded closest chunks into this new list
}

ChunkManager::ChunkPtr ChunkManager::getOwningChunk(const glm::ivec3 &block) const {
    const glm::ivec3 owningChunkPos =
            VecUtils::floor(static_cast<glm::vec3>(block) * (1.0f / Chunk::CHUNK_SIZE));

    const auto it = std::ranges::find_if(chunkSlots, [&](const ChunkSlot &slot) {
        return slot.isBound() && slot.chunk->isLoaded() && slot.chunk->getPos() == owningChunkPos;
    });

    return it != chunkSlots.end()
               ? it->chunk
               : nullptr;
}
