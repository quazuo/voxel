#include "gl-vao.h"

#include "../mesh-context.h"

GLVertexArray::GLVertexArray() {
    glGenVertexArrays(1, &objectID);
    glBindVertexArray(objectID);
}

GLVertexArray::~GLVertexArray() {
    const GLuint objects[1] = { objectID };
    glDeleteVertexArrays(1, objects);
}

void GLVertexArray::enable() {
    glBindVertexArray(objectID);
}

ChunkVertexArray::ChunkVertexArray() :
    vertices(std::make_unique<GLArrayBuffer<glm::vec3>>(0, 3)),
    normals(std::make_unique<GLArrayBuffer<glm::vec3>>(2, 3)),
    uvs(std::make_unique<GLArrayBuffer<glm::vec2>>(1, 2)),
    texIDs(std::make_unique<GLArrayBuffer<int>>(3, 1)),
    indices(std::make_unique<GLElementBuffer>()) {
    glBindVertexArray(0);
}

void ChunkVertexArray::writeToBuffers(const IndexedMeshData &data) const {
    glBindVertexArray(objectID);
    vertices->write(data.vertices);
    uvs->write(data.uvs);
    normals->write(data.normals);
    texIDs->write(data.texIDs);
    indices->write(data.indices);
}

BasicVertexArray::BasicVertexArray() : vertices(std::make_unique<GLArrayBuffer<glm::vec3>>(0, 3)) {
    glBindVertexArray(0);
}

void BasicVertexArray::writeToBuffers(const std::vector<glm::vec3> &data) const {
    vertices->write(data);
}
