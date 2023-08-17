#ifndef MYGE_CHUNK_MANAGER_H
#define MYGE_CHUNK_MANAGER_H

#include <vector>
#include <array>
#include <memory>
#include <map>
#include <cmath>
#include "chunk.h"
#include "src/render/mesh-context.h"
#include "src/utils/size.h"

struct ChunkSlot {
    std::shared_ptr<Chunk> chunk;

    std::shared_ptr<MeshContext> mesh;

    bool _isBound = false;

    void init() {
        mesh = std::make_shared<MeshContext>();
        mesh->initBuffers();
    }

    [[nodiscard]]
    bool isBound() const { return _isBound; }

    void bind(std::shared_ptr<Chunk> c) {
        if (_isBound)
            throw std::runtime_error("tried to call bind() while already bound");

        chunk = c;
        chunk->bindMeshContext(mesh);
        _isBound = true;
    }

    void unbind() {
        if (!_isBound)
            throw std::runtime_error("tried to call unbind() while not bound");

        if (chunk->isLoaded())
            chunk->unload();
        chunk.reset();
        _isBound = false;
    }
};

class ChunkManager {
    using ChunkPtr = std::shared_ptr<Chunk>;

    // list of chunks that are waiting to be loaded
    std::vector<ChunkPtr> loadableChunks; // todo - this can crash! remove this and just get a list of slots or something

    // list of chunks that should be rendered
    std::vector<ChunkPtr> visibleChunks;

    static constexpr int RENDER_DISTANCE = 3;
    static constexpr int GRACE_PERIOD_WIDTH = 1;
    static constexpr int VISIBLE_AREA_WIDTH = 2 * RENDER_DISTANCE + 1 + GRACE_PERIOD_WIDTH;
    std::array<ChunkSlot, SizeUtils::powSize(VISIBLE_AREA_WIDTH, 3)> chunkSlots; // todo - make it into 2 lists: bound/free

    VecUtils::Vec3Discrete lastOccupiedChunkPos = {0, 0, 0};

    std::shared_ptr<OpenGLRenderer> renderer;

    static constexpr size_t MAX_CHUNKS_SERVE_PER_PRAME = 2;

public:
    explicit ChunkManager(const std::shared_ptr<OpenGLRenderer> &rendererPtr) : renderer(rendererPtr) { }

    void init();

    void renderChunks() const;

    void renderChunkOutlines() const;

    void tick();

private:
    void updateChunkSlots();

    void unloadFarChunks(VecUtils::Vec3Discrete currChunkPos);

    void loadNearChunks(VecUtils::Vec3Discrete currChunkPos);

    void updateLoadList();

    void updateRenderList();
};

#endif //MYGE_CHUNK_MANAGER_H
