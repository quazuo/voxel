#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include "block.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/utils/vec.h"

class WorldGen {
public:
    virtual ~WorldGen() = default;

    virtual EBlockType getBlockTypeAt(int x, int y, int z) = 0;

    virtual void setChunkGenCtx(VecUtils::Vec3Discrete chunkPos) = 0;
};

class DefaultWorldGen : public WorldGen {
    noiseutils::NoiseMap heightMap;
    VecUtils::Vec3Discrete chunkPos{};

public:
    EBlockType getBlockTypeAt(int x, int y, int z) override;

    void setChunkGenCtx(VecUtils::Vec3Discrete cPos) override;
};

#endif //VOXEL_WORLD_GEN_H
