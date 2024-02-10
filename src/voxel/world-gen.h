#ifndef VOXEL_WORLD_GEN_H
#define VOXEL_WORLD_GEN_H

#include "block/block.h"
#include "chunk/chunk.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/utils/cube-array.h"

class WorldGen {
    const noise::module::Perlin noiseModule;
    noiseutils::NoiseMap heightMap;

public:
    void fillChunk(const glm::ivec3& chunkPos, CubeArray<Block, Chunk::CHUNK_SIZE>& blockArr);

private:
    void setChunkGenCtx(const glm::ivec3 &chunkPos);
};

#endif //VOXEL_WORLD_GEN_H
