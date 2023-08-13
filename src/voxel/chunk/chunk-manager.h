#ifndef MYGE_CHUNK_MANAGER_H
#define MYGE_CHUNK_MANAGER_H

#include <vector>
#include <array>
#include <memory>
#include <map>
#include "chunk.h"

class ChunkManager {
    using ChunkPtr = std::shared_ptr<Chunk>;

    // list of chunks that are waiting to be loaded
    std::vector<ChunkPtr> loadChunks;

    // list of chunks that are waiting to be unloaded
    std::vector<ChunkPtr> unloadChunks;

    // list of chunks that are relevant, i.e. close enough to the camera to be considered for rendering
    std::vector<ChunkPtr> relevantChunks;

    // list of chunks that should be rendered
    std::vector<ChunkPtr> renderChunks;

    // mapping of (x,y,z) triples (chunk positions) to currently loaded chunks
    std::map<std::pair<int, int>, ChunkPtr> loadedChunks;

    std::shared_ptr<OpenGLRenderer> renderer;

    static constexpr size_t MAX_CHUNKS_SERVE_PER_PRAME = 2;

public:
    explicit ChunkManager(const std::shared_ptr<OpenGLRenderer> &rendererPtr) : renderer(rendererPtr) {
        int max = 2;
        int min = -max;

        for (int x = min; x <= max; x++) {
            for (int y = min; y <= max; y++) {
                for (int z = min; z <= max; z++) {
                    const auto newChunk = std::make_shared<Chunk>(glm::vec3(x, y, z));
                    loadChunks.push_back(newChunk);
                    relevantChunks.push_back(newChunk);
                }
            }
        }
    }

    void render() const;

    void update();

private:
    void updateLoadList();

    void updateUnloadList();

    void updateRelevantList();

    void updateRenderList();

    void renderOutlines() const;
};

#endif //MYGE_CHUNK_MANAGER_H
