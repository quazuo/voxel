#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include <libnoise/noise.h>
#include "block.h"
#include "deps/noiseutils/noiseutils.h"
#include "glm/vec3.hpp"

class WorldGen {
    static noiseutils::NoiseMap heightMap;
    static glm::vec3 chunkPos;

public:
    static EBlockType getBlockTypeAt(int x, int y, int z);

    static void setChunkGenCtx(glm::vec3 chunkPos);
};

#endif //VOXEL_WORLD_GEN_H
