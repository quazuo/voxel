#ifndef MYGE_CHUNK_MANAGER_H
#define MYGE_CHUNK_MANAGER_H

#include <vector>
#include <array>
#include <memory>

#include "chunk.h"

class ChunkManager {
    using ChunkPtr = std::shared_ptr<Chunk>;

    // list of chunks that are waiting to be loaded
    std::vector<ChunkPtr> loadChunks;

    // list of chunks that are waiting to be unloaded
    std::vector<ChunkPtr> unloadChunks;

    // list of chunks that are waiting to have their mesh rebuilt
    std::vector<ChunkPtr> rebuildChunks;

    // list of chunks that are relevant, i.e. close enough to the camera to be considered for rendering
    std::vector<ChunkPtr> relevantChunks;

    // list of chunks that should be rendered
    std::vector<ChunkPtr> renderChunks;

    static constexpr size_t MAX_CHUNKS_SERVE_PER_PRAME = 4;
    static constexpr size_t RENDER_DISTANCE = 4;

public:
    void render(OpenGLRenderer& renderer) const;

    void update();

private:
    void updateLoadList();

    void updateUnloadList();

    void updateRebuildList();

    void updateRelevantList();

    void updateRenderList();
};

#endif //MYGE_CHUNK_MANAGER_H
