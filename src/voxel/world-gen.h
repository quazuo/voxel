#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include <libnoise/noise.h>
#include "src/utils/vec.h"
#include "block.h"

class WorldGen {
public:
    [[nodiscard]]
    static float getHeight(Vec3 chunkPos, int blockX, int blockZ);
};

#endif //VOXEL_WORLD_GEN_H
