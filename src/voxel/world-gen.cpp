#include "world-gen.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/voxel/chunk/chunk.h"

float WorldGen::getHeight(Vec3 chunkPos, int blockX, int blockZ) {
    static const noise::module::Perlin noiseModule;
    noiseutils::NoiseMap heightMap;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretchFactor = 0.3;
    heightMapBuilder.SetBounds(
        stretchFactor * chunkPos.x,
        stretchFactor * (chunkPos.x + 1.0),
        stretchFactor * chunkPos.z,
        stretchFactor * (chunkPos.z + 1.0)
    );
    heightMapBuilder.Build();

    return heightMap.GetValue(blockX, blockZ);
}
