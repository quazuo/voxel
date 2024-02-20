#include "gl-vao.h"

#include "../mesh-context.h"

GLVertexArray::GLVertexArray() {
    glGenVertexArrays(1, &objectID);
    glBindVertexArray(objectID);
}

GLVertexArray::~GLVertexArray() {
    glDeleteVertexArrays(1, &objectID);
}

void GLVertexArray::enable() {
    glBindVertexArray(objectID);
}

ChunksVertexArray::ChunksVertexArray() : posBuffer(std::make_unique<GLArrayBuffer<glm::ivec3> >(0, 3)),
                                         normalUvTexBuffer(std::make_unique<GLArrayBuffer<glm::uint32> >(1, 1)),
                                         indicesBuffer(std::make_unique<GLElementBuffer>()) {
    static_assert(MIN_SECTOR_SIZE % 3 == 0);
    static_assert(MIN_SECTOR_SIZE <= MAX_SECTOR_SIZE);
    glBindVertexArray(0);
}

void ChunksVertexArray::writeChunk(const Chunk::ChunkID chunkID, const IndexedMeshData &mesh) {
    if (mesh.indices.empty()) return;

    glBindVertexArray(objectID);

    const std::vector<glm::uint32> packedNormalUvTexData = mesh.packNormalUvTex();

    const SectorLevel vertexSectorLevel = calcSectorLevel(mesh.vertices.size());
    const SectorLevel indexSectorLevel = calcSectorLevel(mesh.indices.size());

    if (chunkSectorMapping.contains(chunkID)) {
        auto& [vertexSector, indexSector] = chunkSectorMapping.at(chunkID);

        if (vertexSectorLevel == vertexSector.level && indexSectorLevel == indexSector.level) {
            vertexSector.size = mesh.vertices.size();
            indexSector.size = mesh.indices.size();

            const size_t vertexAbsOffset = vertexSector.slabID * SLAB_SIZE + vertexSector.offset;
            const size_t indexAbsOffset = indexSector.slabID * SLAB_SIZE + indexSector.offset;

            posBuffer->write(mesh.vertices.data(), vertexSector.size, vertexAbsOffset);
            normalUvTexBuffer->write(packedNormalUvTexData.data(), vertexSector.size, vertexAbsOffset);
            indicesBuffer->write(mesh.indices.data(), indexSector.size, indexAbsOffset);

            return;
        }

        // todo - don't reclaim both if only one doesn't match
        vertexSlabsState.reclaimSector(vertexSector);
        indexSlabsState.reclaimSector(indexSector);
        chunkSectorMapping.erase(chunkID);
    }

    SectorData vertexSector = vertexSlabsState.requestNewSector(vertexSectorLevel);
    SectorData indexSector = indexSlabsState.requestNewSector(indexSectorLevel);

    vertexSector.size = mesh.vertices.size();
    indexSector.size = mesh.indices.size();

    const size_t vertexAbsOffset = vertexSector.slabID * SLAB_SIZE + vertexSector.offset;
    const size_t indexAbsOffset = indexSector.slabID * SLAB_SIZE + indexSector.offset;

    posBuffer->write(mesh.vertices.data(), vertexSector.size, vertexAbsOffset);
    normalUvTexBuffer->write(packedNormalUvTexData.data(), vertexSector.size, vertexAbsOffset);
    indicesBuffer->write(mesh.indices.data(), indexSector.size, indexAbsOffset);

    const ChunkSectorsData newSectorsData{
        .vertexSector = vertexSector,
        .indexSector = indexSector
    };
    chunkSectorMapping.emplace(chunkID, newSectorsData);
}

void ChunksVertexArray::eraseChunk(const Chunk::ChunkID chunkID) {
    if (!chunkSectorMapping.contains(chunkID)) {
        return;
    }

    const auto &[vertexSector, indexSector] = chunkSectorMapping.at(chunkID);

    vertexSlabsState.reclaimSector(vertexSector);
    indexSlabsState.reclaimSector(indexSector);
}

void ChunksVertexArray::render(const std::vector<Chunk::ChunkID> &targets) const {
    std::vector<GLsizei> counts;
    std::vector<void *> indexOffsets;
    std::vector<GLint> baseIndices;
    size_t actualTargetCount = 0;

    for (const Chunk::ChunkID target: targets) {
        if (!chunkSectorMapping.contains(target)) {
            continue;
        }

        const auto &[vertexSector, indexSector] = chunkSectorMapping.at(target);

        const off_t vertexOffset = static_cast<off_t>(vertexSector.slabID * SLAB_SIZE) + vertexSector.offset;
        const off_t indexOffset = static_cast<off_t>(indexSector.slabID * SLAB_SIZE) + indexSector.offset;

        counts.push_back(static_cast<GLsizei>(indexSector.size));
        indexOffsets.push_back(reinterpret_cast<void *>(indexOffset * sizeof(GLElementBuffer::ElemType)));
        baseIndices.push_back(static_cast<GLint>(vertexOffset));
        actualTargetCount++;
    }

    glMultiDrawElementsBaseVertex(
        GL_TRIANGLES,
        counts.data(),
        GLElementBuffer::getGlElemType(),
        indexOffsets.data(),
        static_cast<GLsizei>(actualTargetCount),
        baseIndices.data()
    );
}

ChunksVertexArray::SectorLevel ChunksVertexArray::calcSectorLevel(const size_t dataSize) {
    return SizeUtils::log(2, dataSize / 3);
}

size_t ChunksVertexArray::calcSectorSize(const SectorLevel level) {
    return MIN_SECTOR_SIZE * SizeUtils::pow(2, level);
}

ChunksVertexArray::SlabID ChunksVertexArray::SlabsState::requestNewSlab() {
    SlabID newID;

    if (freedSlabs.empty()) {
        newID = nextFreshSlab++;
    } else {
        newID = freedSlabs.top();
        freedSlabs.pop();
    }

    usedSlabs.emplace(newID, SlabData());
    return newID;
}

ChunksVertexArray::SectorData ChunksVertexArray::SlabsState::requestNewSector(const SectorLevel level) {
    for (auto &[id, data]: usedSlabs) {
        const auto res = requestSectorFromSlab(data, id, level);
        if (res) return *res;
    }

    const SlabID id = requestNewSlab();
    SlabData &data = usedSlabs.at(id);
    const auto res = requestSectorFromSlab(data, id, level);

    if (!res) {
        throw std::runtime_error("unexpected result from requestSectorFromSlab when passing a fresh slab");
    }

    return *res;
}

void ChunksVertexArray::SlabsState::reclaimSector(const SectorData &sector) {
    auto &slabLevel = usedSlabs.at(sector.slabID).levels[sector.level];

    const size_t levelSize = calcSectorSize(sector.level);
    const size_t sectorIndex = sector.offset / levelSize;
    const bool isLeftBuddy = sectorIndex % 2 == 0;
    const off_t buddyOffset = isLeftBuddy
                                  ? sector.offset + static_cast<off_t>(levelSize)
                                  : sector.offset - static_cast<off_t>(levelSize);

    if (slabLevel.contains(buddyOffset)) {
        slabLevel.erase(buddyOffset);
        usedSlabs.at(sector.slabID).levels[sector.level + 1].emplace(isLeftBuddy ? sector.offset : buddyOffset);

    } else {
        slabLevel.emplace(sector.offset);
    }
}

std::optional<ChunksVertexArray::SectorData>
ChunksVertexArray::SlabsState::requestSectorFromSlab(SlabData &slabData, const SlabID slabID,
                                                     const SectorLevel level) const {
    if (slabData.levels[level].empty()) {
        SectorLevel dividedSectorLevel = level + 1;
        while (dividedSectorLevel < SECTOR_LVL_COUNT && slabData.levels[dividedSectorLevel].empty()) {
            dividedSectorLevel++;
        }

        if (dividedSectorLevel == SECTOR_LVL_COUNT) {
            return {};
        }

        for (SectorLevel currLevel = dividedSectorLevel; currLevel > level; currLevel--) {
            const off_t offset = *slabData.levels[currLevel].begin();
            const size_t nextLevelSize = 3 * SizeUtils::pow(2, currLevel - 1);

            slabData.levels[currLevel].erase(offset);
            slabData.levels[currLevel - 1].emplace(offset);
            slabData.levels[currLevel - 1].emplace(offset + static_cast<off_t>(nextLevelSize));
        }
    }

    const off_t offset = *slabData.levels[level].begin();
    slabData.levels[level].erase(offset);

    return SectorData{
        .slabID = slabID,
        .offset = offset,
        .level = level,
        .size = 0 // filled in later
    };
}

BasicVertexArray::BasicVertexArray() : vertices(std::make_unique<GLArrayBuffer<glm::vec3> >(0, 3)) {
    glBindVertexArray(0);
}

void BasicVertexArray::writeToBuffers(const std::vector<glm::vec3> &data) const {
    vertices->write(data.data(), data.size(), 0);
}
