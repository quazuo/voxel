#include "world-gen.h"
#include "src/voxel/chunk/chunk.h"

EBlockType DefaultWorldGen::getBlockTypeAt(const int x, const int y, const int z) {
    const float height = heightMap.GetValue(x, z) * Chunk::CHUNK_SIZE;
    const int threshold = static_cast<int>(height);
    const int absY = chunkPos.y * Chunk::CHUNK_SIZE + y;

    if (absY > threshold)
        return BlockType_None;
    if (absY == threshold)
        return BlockType_Grass;

    constexpr int dirtHeight = 5;
    if (absY + dirtHeight < threshold)
        return BlockType_Stone;

    return BlockType_Dirt;
}

void DefaultWorldGen::setChunkGenCtx(const glm::ivec3 cPos) {
    chunkPos = cPos;
    heightMap = {};

    const noise::module::Perlin noiseModule;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretchFactor = 0.1;
    heightMapBuilder.SetBounds(
        stretchFactor * static_cast<double>(cPos.x),
        stretchFactor * static_cast<double>(cPos.x + 1),
        stretchFactor * static_cast<double>(cPos.z),
        stretchFactor * static_cast<double>(cPos.z + 1)
    );
    heightMapBuilder.Build();
}
