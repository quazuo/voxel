#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include "block/block.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/utils/vec.h"

class WorldGen {
public:
    virtual ~WorldGen() = default;

    virtual EBlockType getBlockTypeAt(int x, int y, int z) = 0;

    virtual void setChunkGenCtx(glm::ivec3 chunkPos) = 0;
};

class DefaultWorldGen : public WorldGen {
    noiseutils::NoiseMap heightMap;
    glm::ivec3 chunkPos{};

public:
    EBlockType getBlockTypeAt(int x, int y, int z) override;

    void setChunkGenCtx(glm::ivec3 cPos) override;
};

#endif //VOXEL_WORLD_GEN_H
