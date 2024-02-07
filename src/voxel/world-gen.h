#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include <optional>

#include "block/block.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/utils/vec.h"

class WorldGen {
    noiseutils::NoiseMap heightMap;
    std::optional<glm::ivec3> chunkPos{};

public:
    EBlockType getBlockTypeAt(const glm::ivec3& blockPos) const;

    void setChunkGenCtx(const glm::ivec3 &cPos);
};

#endif //VOXEL_WORLD_GEN_H
