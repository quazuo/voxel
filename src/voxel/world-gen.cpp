#include "world-gen.h"
#include "src/voxel/chunk/chunk.h"

EBlockType WorldGen::getBlockTypeAt(const glm::ivec3& blockPos) const {
    constexpr float stretch = 2.5f;
    const float height = heightMap.GetValue(blockPos.x, blockPos.z) * Chunk::CHUNK_SIZE * stretch;
    const int threshold = static_cast<int>(height);
    const int absY = chunkPos->y * Chunk::CHUNK_SIZE + blockPos.y;

    if (absY > threshold)
        return BlockType_None;
    if (absY == threshold)
        return BlockType_Grass;

    constexpr int dirtHeight = 5;
    if (absY + dirtHeight < threshold)
        return BlockType_Stone;

    return BlockType_Dirt;
}

void WorldGen::setChunkGenCtx(const glm::ivec3 &cPos) {
    chunkPos = cPos;
    heightMap = {};

    const noise::module::Perlin noiseModule;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretch = 0.1;
    heightMapBuilder.SetBounds(
        stretch * static_cast<double>(cPos.x),
        stretch * static_cast<double>(cPos.x + 1),
        stretch * static_cast<double>(cPos.z),
        stretch * static_cast<double>(cPos.z + 1)
    );
    heightMapBuilder.Build();
}
