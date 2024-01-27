#ifndef MYGE_CHUNK_MANAGER_H
#define MYGE_CHUNK_MANAGER_H

#include <utility>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <cmath>
#include "chunk.h"
#include "src/render/mesh-context.h"
#include "src/utils/size.h"

/**
 * Class responsible for managing chunks in the world -- most importantly
 * loading and unloading them dynamically.
 */
class ChunkManager {
    using ChunkPtr = std::shared_ptr<Chunk>;

    struct ChunkSlot {
        std::shared_ptr<Chunk> chunk;
        std::shared_ptr<ChunkMeshContext> mesh = std::make_shared<ChunkMeshContext>();

        [[nodiscard]]
        bool isBound() const { return chunk != nullptr; }

        void bind(std::shared_ptr<Chunk> c);

        void unbind();
    };

    // list of chunks that are waiting to be loaded
    std::vector<ChunkPtr> loadableChunks;

    // list of chunks that should be rendered
    std::vector<ChunkPtr> visibleChunks;

    // how many chunks around the camera should always be loaded
    static constexpr int RENDER_DISTANCE = 3;

    /*
     * this prevents jittering around a chunk's border to cause chunks to be repeatedly loaded and unloaded.
     * basically, chunks get unloaded only if they are `RENDER_DISTANCE + GRACE_PERIOD_WIDTH` chunks away from
     * the camera.
     */
    static constexpr int GRACE_PERIOD_WIDTH = 1;

    static constexpr int VISIBLE_AREA_WIDTH = 2 * RENDER_DISTANCE + 1 + GRACE_PERIOD_WIDTH;

    std::array<ChunkSlot, SizeUtils::pow(VISIBLE_AREA_WIDTH, 3)> chunkSlots;

    VecUtils::Vec3Discrete lastOccupiedChunkPos = {0, 0, 0};

    std::shared_ptr<OpenGLRenderer> renderer;
    std::shared_ptr<class WorldGen> worldGen;

    // this limits how many chunks can be handled each frame to prevent big stutters
    static constexpr size_t MAX_CHUNKS_SERVE_PER_PRAME = 2;

public:
    explicit ChunkManager(std::shared_ptr<OpenGLRenderer> r, std::shared_ptr<WorldGen> wg);

    void tick();

    /**
     * Renders all the currently visible chunks. Whether a chunk is visible or not
     * is mostly determined by the `RENDER_DISTANCE` and `GRACE_PERIOD_WIDTH` constants.
     */
    void renderChunks() const;

    /**
     * Renders outlines around all chunks in `RENDER_DISTANCE` around the camera.
     */
    void renderChunkOutlines() const;

    /**
     * Finds which nearest block is under the camera's crosshair.
     *
     * @param lookedAtBlocks List of block coordinates occupying space under the crosshair.
     * @return If any, the coordinates of the requested block.
     */
    [[nodiscard]]
    std::optional<glm::vec3> getTargetedBlock(const std::vector<VecUtils::Vec3Discrete> &lookedAtBlocks) const;

    /**
     * Updates a specific block at absolute coordinates to be a set type.
     *
     * @param block The block's coordinates.
     * @param type Type of which the block should now be.
     */
    void updateBlock(VecUtils::Vec3Discrete block, EBlockType type) const;

private:
    /**
     * Finds which chunk a given block belongs to.
     *
     * @param block The block's coordinates.
     * @return Pointer to the owning chunk.
     */
    [[nodiscard]]
    ChunkPtr getOwningChunk(VecUtils::Vec3Discrete block) const;

    /**
     * Checks if any chunks should be loaded or unloaded, and does so if that's the case.
     */
    void updateChunkSlots();

    /**
     * Unloads all chunks that are at least `RENDER_DISTANCE + GRACE_PERIOD_WIDTH` chunks' worth of distance
     * away from the camera's position.
     *
     * @param currChunkPos Position of the chunk the camera currently resides in,
     * given by the position of the chunk's vertex with the lowest coordinates.
     */
    void unloadFarChunks(VecUtils::Vec3Discrete currChunkPos);

    /**
     * Loads all chunks that are within `RENDER_DISTANCE` chunks' worth of distance from the camera's position.
     *
     * @param currChunkPos Position of the chunk the camera currently resides in,
     * given by the position of the chunk's vertex with the lowest coordinates.
     */
    void loadNearChunks(VecUtils::Vec3Discrete currChunkPos);

    /**
     * Takes up to `MAX_CHUNKS_SERVE_PER_PRAME` chunks from the `loadableChunks` list and loads them.
     */
    void updateLoadList();

    /**
     * Updates which chunks are actually visible and renderrable.
     */
    void updateRenderList();
};

#endif //MYGE_CHUNK_MANAGER_H
