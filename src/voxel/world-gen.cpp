#include "world-gen.h"
#include "src/voxel/chunk/chunk.h"

noiseutils::NoiseMap WorldGen::heightMap;
glm::vec3 WorldGen::chunkPos;

EBlockType WorldGen::getBlockTypeAt(const int x, const int y, const int z) {
//    const float height = (float) Chunk::CHUNK_SIZE / 2 + std::clamp(
//        heightMap.GetValue(x, z) * Chunk::CHUNK_SIZE / 2,
//        (float) -Chunk::CHUNK_SIZE / 2 + 1,
//        (float) Chunk::CHUNK_SIZE / 2
//    );

    const float height = heightMap.GetValue(x, z) * Chunk::CHUNK_SIZE;

    const int threshold = (int) height;
    const int absY = (int) chunkPos.y * Chunk::CHUNK_SIZE + y;

    if (absY > threshold)
        return EBlockType::BlockType_None;
    if (absY == threshold)
        return EBlockType::BlockType_Grass;
    return EBlockType::BlockType_Dirt;
}

void WorldGen::setChunkGenCtx(glm::vec3 cPos) {
    chunkPos = cPos;
    heightMap = {};

    const noise::module::Perlin noiseModule;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretchFactor = 0.3;
    heightMapBuilder.SetBounds(
        stretchFactor * (double) chunkPos.x,
        stretchFactor * (double) (chunkPos.x + 1),
        stretchFactor * (double) chunkPos.z,
        stretchFactor * (double) (chunkPos.z + 1)
    );
    heightMapBuilder.Build();
}
