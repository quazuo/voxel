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

ChunkVertexArray::ChunkVertexArray() :
    packedVertices(std::make_unique<GLArrayBuffer<glm::uint32>>(0, 1)),
    texIDs(std::make_unique<GLArrayBuffer<int>>(1, 1)),
    indices(std::make_unique<GLElementBuffer>()) {
    glBindVertexArray(0);
}

static glm::uint32 getFaceIndex(const EBlockFace face) {
    switch (face) {
        case Front:
            return 0;
        case Back:
            return 1;
        case Right:
            return 2;
        case Left:
            return 3;
        case Top:
            return 4;
        case Bottom:
            return 5;
        case N_FACES:
            throw std::runtime_error("invalid switch branch in ChunkVertexArray::writeToBuffers");
    }
}

void ChunkVertexArray::writeToBuffers(const IndexedMeshData &data) const {
    glBindVertexArray(objectID);

    std::vector<glm::uint32> packedVerticesData;

    for (size_t i = 0; i < data.vertices.size(); i++) {
        glm::uint32 packedVertex = 0;

        packedVertex |= (static_cast<glm::uint32>(data.vertices[i].x) & 0x1F) << 27;
        packedVertex |= (static_cast<glm::uint32>(data.vertices[i].y) & 0x1F) << 22;
        packedVertex |= (static_cast<glm::uint32>(data.vertices[i].z) & 0x1F) << 17;

        const glm::uint32 faceIndex = getFaceIndex(getFaceFromNormal(data.normals[i]));
        packedVertex |= (faceIndex & 0x7) << 14;

        packedVertex |= (static_cast<glm::uint32>(data.uvs[i].x) & 0x1F) << 9;
        packedVertex |= (static_cast<glm::uint32>(data.uvs[i].y) & 0x1F) << 4;

        packedVerticesData.push_back(packedVertex);
    }

    packedVertices->write(packedVerticesData);
    texIDs->write(data.texIDs);
    indices->write(data.indices);
}

BasicVertexArray::BasicVertexArray() : vertices(std::make_unique<GLArrayBuffer<glm::vec3>>(0, 3)) {
    glBindVertexArray(0);
}

void BasicVertexArray::writeToBuffers(const std::vector<glm::vec3> &data) const {
    vertices->write(data);
}
