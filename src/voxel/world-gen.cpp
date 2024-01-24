#include "world-gen.h"
#include "src/voxel/chunk/chunk.h"

EBlockType DefaultWorldGen::getBlockTypeAt(const int x, const int y, const int z) {
    const float height = heightMap.GetValue(x, z) * Chunk::CHUNK_SIZE;
    const int threshold = (int) height;
    const int absY = chunkPos.y * Chunk::CHUNK_SIZE + y;

    if (absY > threshold)
        return EBlockType::BlockType_None;
    if (absY == threshold)
        return EBlockType::BlockType_Grass;

    constexpr int dirtHeight = 5;
    if (absY + dirtHeight < threshold)
        return EBlockType::BlockType_Stone;

    return EBlockType::BlockType_Dirt;
}

void DefaultWorldGen::setChunkGenCtx(VecUtils::Vec3Discrete cPos) {
    chunkPos = cPos;
    heightMap = {};

    const noise::module::Perlin noiseModule;
    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretchFactor = 0.1;
    heightMapBuilder.SetBounds(
        stretchFactor * (double) cPos.x,
        stretchFactor * (double) (cPos.x + 1),
        stretchFactor * (double) cPos.z,
        stretchFactor * (double) (cPos.z + 1)
    );
    heightMapBuilder.Build();
}
