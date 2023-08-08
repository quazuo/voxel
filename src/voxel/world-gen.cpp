#include "world-gen.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/voxel/chunk/chunk.h"

float WorldGen::getHeight(Vec3 chunkPos, int blockX, int blockZ) const {
    (void) chunkPos;

    noiseutils::NoiseMap heightMap;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;
    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    heightMapBuilder.SetBounds(blockX, blockX + 1.0, blockZ, blockZ + 1.0);
    heightMapBuilder.Build();

    return heightMap.GetValue(blockX, blockZ);
}
