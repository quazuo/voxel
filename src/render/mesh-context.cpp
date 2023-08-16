#include "mesh-context.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

void MeshContext::clear() {
    triangles = {};
    indexedData = {};
    isIndexed = false;
}

void MeshContext::addTriangle(PackedVertex &vertex1, PackedVertex &vertex2, PackedVertex &vertex3) {
    triangles.emplace_back(vertex1, vertex2, vertex3);
}

void MeshContext::indexVertex(const PackedVertex &vertex, IndexedMeshData &data,
                              std::map<PackedVertex, unsigned short> &vertexToOutIndex) {
    int index = -1;
    auto it = vertexToOutIndex.find(vertex);
    if (it != vertexToOutIndex.end())
        index = it->second;

    bool found = index != -1;

    if (found) { // A similar vertex is already in the VBO, use it instead!
        data.indices.push_back(index);

    } else { // If not, it needs to be added in the output data.
        data.vertices.push_back(vertex.position);
        data.uvs.push_back(vertex.uv);
        data.normals.push_back(vertex.normal);
        unsigned short newIndex = (unsigned short) data.vertices.size() - 1;
        data.indices.push_back(newIndex);
        vertexToOutIndex[vertex] = newIndex;
    }
}

void MeshContext::makeIndexed() {
    if (isIndexed)
        return;

    std::map<PackedVertex, unsigned short> vertexToOutIndex;

    for (const auto &[v1, v2, v3]: triangles) {
        indexVertex(v1, indexedData, vertexToOutIndex);
        indexVertex(v2, indexedData, vertexToOutIndex);
        indexVertex(v3, indexedData, vertexToOutIndex);
    }

    isIndexed = true;
}

void MeshContext::initBuffers() {
    glGenBuffers(1, &vertexBufferID);
    glGenBuffers(1, &uvBufferID);
    glGenBuffers(1, &normalBufferID);
    glGenBuffers(1, &elementBufferID);

    // current upper limit, reached for chunks in a checkerboard pattern.
    // this NEEDS to be optimized asap, as it drains way too much RAM.
    size_t bufferSize = 73728;

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize * sizeof(unsigned short), nullptr, GL_DYNAMIC_DRAW);
}

GLuint MeshContext::getBufferID(MeshContext::EBufferType bufferType) const {
    switch (bufferType) {
        case EBufferType::Vertex:
            return vertexBufferID;
        case EBufferType::UV:
            return uvBufferID;
        case EBufferType::Normal:
            return normalBufferID;
        case EBufferType::Element:
        default:
            return elementBufferID;
    }
}

void MeshContext::freeBuffers() {
    constexpr size_t nBuffers = 4;
    const GLuint buffers[nBuffers] = {vertexBufferID, uvBufferID, normalBufferID, elementBufferID};
    glDeleteBuffers(nBuffers, buffers);
}
