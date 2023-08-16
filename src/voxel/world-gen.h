#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include <libnoise/noise.h>
#include "block.h"
#include "deps/noiseutils/noiseutils.h"
#include "glm/vec3.hpp"
#include "src/utils/vec.h"

class WorldGen {
    static noiseutils::NoiseMap heightMap;
    static VecUtils::Vec3Discrete chunkPos;

public:
    static EBlockType getBlockTypeAt(int x, int y, int z);

    static void setChunkGenCtx(VecUtils::Vec3Discrete chunkPos);
};

#endif //VOXEL_WORLD_GEN_H
