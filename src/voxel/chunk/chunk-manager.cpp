#include <utility>

#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glm/gtx/norm.hpp"

#include "chunk-manager.h"

#include "src/render/renderer.h"
#include "src/render/gui.h"
#include "src/utils/size.h"

void ChunkManager::ChunkSlot::bind(std::unique_ptr<Chunk>&& c) {
    if (isBound())
        throw std::runtime_error("tried to call bind() while already bound");

    chunk = std::move(c);
}

void ChunkManager::ChunkSlot::unbind() {
    if (!isBound())
        throw std::runtime_error("tried to call unbind() while not bound");

    if (chunk->isLoaded()) {
        chunk->unload();
    }

    chunk = nullptr;
}

ChunkManager::ChunkManager(std::shared_ptr<OpenGLRenderer> r, std::shared_ptr<WorldGen> wg)
    : renderer(std::move(r)), worldGen(std::move(wg)) {

    const size_t visibleAreaWidth = 2 * renderDistance + gracePeriodWidth + 1;
    for (size_t i = 0; i < SizeUtils::pow(visibleAreaWidth, 3); i++) {
        chunkSlots.emplace_back(std::make_shared<ChunkSlot>());
    }

    loadNearChunks();
    sortChunkSlots(loadableChunks);
}

void ChunkManager::renderChunks() const {
    // if (renderer->shouldDrawShadows()) {
    //     renderer->startRenderingShadowMap();
    //
    //     for (const auto &slot: chunkSlots) {
    //         if (!slot->isBound())
    //             continue;
    //         if (!slot->chunk->shouldRender())
    //             continue;
    //
    //         if (slot->chunk->isDirty()) {
    //             makeChunkMesh(*slot);
    //         }
    //
    //         renderer->makeChunkShadowMap(*slot->mesh);
    //     }
    //
    //     renderer->finishRenderingShadowMap();
    // }

    std::vector<Chunk::ChunkID> targets;

    for (const auto &slot: visibleChunks) {
        if (slot->chunk->isDirty()) {
            makeChunkMesh(*slot);
        }

        targets.push_back(slot->chunk->getID());
    }

    renderer->renderChunks(targets);
}

void ChunkManager::renderChunkOutlines() const {
    for (auto &slot: chunkSlots) {
        if (!slot->isBound())
            continue;

        OpenGLRenderer::LineType lineType = OpenGLRenderer::CHUNK_OUTLINE;
        if (std::ranges::find(visibleChunks, slot) == visibleChunks.end()) {
            lineType = OpenGLRenderer::EMPTY_CHUNK_OUTLINE;
        } else {
            renderer->addChunkOutline(slot->chunk->getPos() * Chunk::CHUNK_SIZE, lineType);
        }
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
        if (ImGui::ArrowButton("cm_left1", ImGuiDir_Left)) { setRenderDistance(std::max(renderDistance - 1, 0)); }
        ImGui::SameLine();
        if (ImGui::ArrowButton("cm_right1", ImGuiDir_Right)) { setRenderDistance(renderDistance + 1); }

        ImGui::Text("Chunks served per frame: %d ", chunksServePerFrame);
        ImGui::SameLine();
        if (ImGui::ArrowButton("cm_left2", ImGuiDir_Left)) { chunksServePerFrame = std::max(chunksServePerFrame - 1, 0); }
        ImGui::SameLine();
        if (ImGui::ArrowButton("cm_right2", ImGuiDir_Right)) { chunksServePerFrame++; }

        ImGui::Text("Loadable chunks: %d", loadableChunks.size());
        ImGui::Text("Visible chunks: %d", visibleChunks.size());
        ImGui::Text("Chunk slots: %d", chunkSlots.size());
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
    for (const auto &slot: chunkSlots) {
        if (!slot->isBound()) continue;

        const glm::ivec3 chunkPosDist = VecUtils::abs(slot->chunk->getPos() - lastOccupiedChunkPos);
        const bool isOutsideRenderDistance = VecUtils::any<float>(
            chunkPosDist,
            [&](const float x) { return x > static_cast<float>(renderDistance + gracePeriodWidth); }
        );

        if (isOutsideRenderDistance) {
            renderer->freeChunkMesh(slot->chunk->getID());
            slot->unbind();
        }
    }

    erase_if(loadableChunks, [&](const ChunkSlotPtr &slot) {
        return !slot->isBound() || VecUtils::any<float>(
            VecUtils::abs(slot->chunk->getPos() - lastOccupiedChunkPos),
            [&](const float x) { return x > static_cast<float>(renderDistance + gracePeriodWidth); }
        );
    });
}

void ChunkManager::loadNearChunks() {
    const size_t newChunkLoadCubeWidth = 2 * renderDistance + 1;
    CubeVector<uint8_t> loadedChunksMap{newChunkLoadCubeWidth}; // uint8_t instead of bool because vector<bool> sucks

    // check which positions, relative to ours, are occupied by loaded chunks
    for (const auto &slot: chunkSlots) {
        if (!slot->isBound()) continue;

        const glm::ivec3 relPos = slot->chunk->getPos() - lastOccupiedChunkPos;

        // relPos shifted so that it's non-negative
        const glm::ivec3 shiftedRelPos = relPos + renderDistance;

        // we're interested only in positions within render distance -- we don't care about chunks that are
        // still loaded only because they're in the grace period
        if (VecUtils::all<int>(shiftedRelPos, [&](const int x) { return x < newChunkLoadCubeWidth; })) {
            loadedChunksMap[shiftedRelPos] = true;
        }
    }

    // use the previously gathered information to load missing chunks to free slots
    auto slotIt = chunkSlots.begin();

    loadedChunksMap.forEach([&](const size_t x, const size_t y, const size_t z, const uint8_t &isLoaded) {
        if (isLoaded) return;

        // chunk at `newChunkPos` is unloaded but should be -- we'll load it
        const glm::ivec3 newChunkPos = lastOccupiedChunkPos + glm::ivec3(x, y, z) - renderDistance;

        // find a free slot and bind it with the new chunk
        while ((*slotIt)->isBound()) {
            slotIt++;
        }

        (*slotIt)->bind(std::make_unique<Chunk>(nextFreeID++, newChunkPos));
        loadableChunks.push_back(*slotIt);
    });

    sortChunkSlots(loadableChunks);
}

void ChunkManager::sortChunkSlots(std::vector<ChunkSlotPtr>& chunks) const {
    const glm::vec3 cameraPos = renderer->getCameraPos();

    std::ranges::sort(chunks, [&](const ChunkSlotPtr &a, const ChunkSlotPtr &b) {
        const auto aDist = glm::length2(cameraPos - static_cast<glm::vec3>(a->chunk->getPos() * Chunk::CHUNK_SIZE));
        const auto bDist = glm::length2(cameraPos - static_cast<glm::vec3>(b->chunk->getPos() * Chunk::CHUNK_SIZE));
        return aDist < bDist;
    });
}

void ChunkManager::updateLoadList() {
    int nChunksLoaded = 0;

    for (const ChunkSlotPtr &slot: loadableChunks) {
        if (nChunksLoaded == chunksServePerFrame) {
            break;
        }

        if (!slot->chunk->isLoaded()) {
            slot->chunk->generate(*worldGen); // todo - this should load from disk, not always generate.
            makeChunkMesh(*slot);
            nChunksLoaded++;
        }
    }

    erase_if(loadableChunks, [](const ChunkSlotPtr &slot) { return slot->chunk->isLoaded(); });
}

void ChunkManager::updateRenderList() {
    // clear the render list each frame BEFORE we do our tests to see what chunks should be rendered
    visibleChunks.clear();

    for (auto &slot: chunkSlots) {
        if (!slot->isBound())
            continue;
        if (!slot->chunk->shouldRender()) // early flags check, so we don't always have to do the frustum check...
            continue;
        if (!renderer->isChunkInFrustum(*slot->chunk))
            continue;

        visibleChunks.push_back(slot);
    }
}

std::optional<glm::ivec3> ChunkManager::getTargetedBlock(const std::vector<glm::ivec3> &lookedAtBlocks) const {
    for (auto &block: lookedAtBlocks) {
        const Chunk* chunk = getOwningChunk(block);
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
    Chunk* chunk = getOwningChunk(block);
    if (!chunk) return;

    const glm::ivec3 relativeBlockPos = block - chunk->getPos() * Chunk::CHUNK_SIZE;
    chunk->updateBlock(relativeBlockPos, type);
}

void ChunkManager::setRenderDistance(const int newRenderDistance) {
    if (renderDistance == newRenderDistance) return;

    const size_t visibleAreaWidth = 2 * newRenderDistance + gracePeriodWidth + 1;
    const size_t newSlotsCount = SizeUtils::pow(visibleAreaWidth, 3);

    if (newRenderDistance > renderDistance) {
        const size_t slotsToAdd = newSlotsCount - chunkSlots.size();

        for (size_t i = 0; i < slotsToAdd; i++) {
            chunkSlots.emplace_back(std::make_shared<ChunkSlot>());
        }

    } else {
        chunkSlots.clear();
        loadableChunks.clear();
        visibleChunks.clear();

        for (size_t i = 0; i < newSlotsCount; i++) {
            chunkSlots.emplace_back(std::make_shared<ChunkSlot>());
        }

        // todo - copy already loaded closest chunks into this new list instead of wiping everything
        // sortChunkSlots(chunkSlots);
        // chunkSlots.resize(newSlotsCount);`
    }

    renderDistance = newRenderDistance;
    loadNearChunks();
}

Chunk* ChunkManager::getOwningChunk(const glm::ivec3 &block) const {
    const glm::ivec3 owningChunkPos = VecUtils::floor(static_cast<glm::vec3>(block) * (1.0f / Chunk::CHUNK_SIZE));

    const auto it = std::ranges::find_if(chunkSlots, [&](const ChunkSlotPtr &slot) {
        return slot->isBound() && slot->chunk->isLoaded() && slot->chunk->getPos() == owningChunkPos;
    });

    if (it == chunkSlots.end()) {
        return nullptr;
    }

    return (*it)->chunk.get();
}

void ChunkManager::makeChunkMesh(const ChunkSlot &slot) const {
    ChunkMeshContext& meshCtx = *slot.mesh;
    slot.chunk->createMesh(meshCtx, renderer->getTextureManager());
    renderer->writeChunkMesh(slot.chunk->getID(), meshCtx.getIndexedData());
    meshCtx.clear();
}
