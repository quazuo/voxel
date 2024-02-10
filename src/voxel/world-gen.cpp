#include "world-gen.h"
#include "src/voxel/chunk/chunk.h"

void WorldGen::fillChunk(const glm::ivec3 &chunkPos, CubeArray<Block, Chunk::CHUNK_SIZE> &blockArr) {
    setChunkGenCtx(chunkPos);

    const int chunkAbsY = chunkPos.y * Chunk::CHUNK_SIZE;

    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
            constexpr float stretch = 2.5f;
            const float height = heightMap.GetValue(x, z) * Chunk::CHUNK_SIZE * stretch;
            const int threshold = static_cast<int>(height);

            for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
                constexpr int dirtHeight = 5;
                const int absY = chunkAbsY + y;

                if (absY + dirtHeight < threshold) {
                    blockArr[x][y][z].blockType = BlockType_Stone;

                } else if (absY < threshold) {
                    blockArr[x][y][z].blockType = BlockType_Dirt;

                } else if (absY == threshold) {
                    blockArr[x][y][z].blockType = BlockType_Grass;

                } else {
                    blockArr[x][y][z].blockType = BlockType_None;
                }
            }
        }
    }
}

void WorldGen::setChunkGenCtx(const glm::ivec3 &chunkPos) {
    heightMap = {};

    noiseutils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(noiseModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);
    heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    constexpr double stretch = 0.1;
    heightMapBuilder.SetBounds(
        stretch * static_cast<double>(chunkPos.x),
        stretch * static_cast<double>(chunkPos.x + 1),
        stretch * static_cast<double>(chunkPos.z),
        stretch * static_cast<double>(chunkPos.z + 1)
    );
    heightMapBuilder.Build();
}
