#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include <libnoise/noise.h>
#include "src/utils/vec.h"
#include "block.h"

class WorldGen {
    noise::module::Perlin noiseModule;

public:
    [[nodiscard]]
    float getHeight(Vec3 chunkPos, int blockX, int blockZ) const;
};

#endif //VOXEL_WORLD_GEN_H
