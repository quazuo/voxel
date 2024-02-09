#ifndef MYGE_CHUNK_MANAGER_H
#define MYGE_CHUNK_MANAGER_H

#include <vector>
#include <memory>
#include "chunk.h"
#include "src/render/mesh-context.h"
#include "src/utils/vec.h"

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
    int renderDistance = 3;

    /*
     * this prevents jittering around a chunk's border to cause chunks to be repeatedly loaded and unloaded.
     * basically, chunks get unloaded only if they are `RENDER_DISTANCE + GRACE_PERIOD_WIDTH` chunks away from
     * the camera.
     */
    int gracePeriodWidth = 1;

    /*
     * list of slots in which currently loaded (or only loadable) chunks may reside.
     * this should always be of size `2 * renderDistance + gracePeriodWidth + 1`.
     */
    std::vector<ChunkSlot> chunkSlots;

    // optional because we want to specially handle the si
    glm::ivec3 lastOccupiedChunkPos = {0, 0, 0};

    std::shared_ptr<OpenGLRenderer> renderer;
    std::shared_ptr<class WorldGen> worldGen;

    // this limits how many chunks can be handled each frame to prevent big stutters
    static constexpr size_t MAX_CHUNKS_SERVE_PER_PRAME = 2;

public:
    explicit ChunkManager(std::shared_ptr<OpenGLRenderer> r, std::shared_ptr<WorldGen> wg);

    void tick();

    void renderGuiSection();

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
    std::optional<glm::ivec3> getTargetedBlock(const std::vector<glm::ivec3> &lookedAtBlocks) const;

    /**
     * Updates a specific block at absolute coordinates to be a set type.
     *
     * @param block The block's coordinates.
     * @param type Type of which the block should now be.
     */
    void updateBlock(const glm::ivec3 &block, EBlockType type) const;

    /**
     * Updates the render distance.
     * This clears all loaded chunks and as a consequence needs them to be reloaded later.
     */
    void setRenderDistance(int newRenderDistance);

private:
    /**
     * Finds which chunk a given block belongs to.
     *
     * @param block The block's coordinates.
     * @return Pointer to the owning chunk.
     */
    [[nodiscard]]
    ChunkPtr getOwningChunk(const glm::ivec3 &block) const;

    /**
     * Checks if any chunks should be loaded or unloaded, and does so if that's the case.
     */
    void updateChunkSlots();

    /**
     * Unloads all chunks that are at least `RENDER_DISTANCE + GRACE_PERIOD_WIDTH` chunks' worth of distance
     * away from the camera's position.
     */
    void unloadFarChunks();

    /**
     * Loads all chunks that are within `RENDER_DISTANCE` chunks' worth of distance from the camera's position.
     */
    void loadNearChunks();

    /**
     * Sorts the loadable so that the chunks closest to the camera get loaded first.
     */
    void sortLoadList();

    /**
     * Takes up to `MAX_CHUNKS_SERVE_PER_PRAME` chunks from the `loadableChunks` list and loads them.
     */
    void updateLoadList();

    bool isCompletelyOccluded(const ChunkPtr& chunk) const;

    /**
     * Updates which chunks are actually visible and renderrable.
     */
    void updateRenderList();
};

#endif //MYGE_CHUNK_MANAGER_H
