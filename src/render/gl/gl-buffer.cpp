#include "gl-buffer.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

template<typename T>
GLBuffer<T>::~GLBuffer() {
    glDeleteBuffers(1, &bufferID);
}

template<typename T>
void GLBuffer<T>::updateBufferCapacity(const GLsizeiptr dataSize) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);

    if (dataSize > capacity) {
        // buffer too small, need to expand it
        GLsizeiptr newCapacity = capacity;

        while (dataSize > newCapacity) {
            newCapacity *= 2;
        }

        capacity = newCapacity;
        glBufferData(GL_ARRAY_BUFFER, newCapacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);

    } else if (dataSize <= capacity / 4 && capacity != BASE_CAPACITY) {
        // buffer too large, will shrink it for optimization
        GLsizeiptr newCapacity = capacity;

        while (dataSize <= newCapacity / 4 && newCapacity != BASE_CAPACITY) {
            newCapacity /= 2;
        }

        capacity = newCapacity;
        glBufferData(GL_ARRAY_BUFFER, newCapacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }
}

template<typename T>
GLArrayBuffer<T>::GLArrayBuffer(const GLuint index, const GLint count) : bufferIndex(index), compCount(count) {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glVertexAttribPointer(bufferIndex, compCount, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, this->BASE_CAPACITY * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(bufferIndex);
}

template<typename T>
void GLArrayBuffer<T>::write(const std::vector<T> &data) {
    this->size = data.size();
    this->updateBufferCapacity(this->size);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, this->size * sizeof(T), data.data());
}

template<typename T>
void GLArrayBuffer<T>::enable() {
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
}

template<typename T>
void GLArrayBuffer<T>::disable() const {
    glDisableVertexAttribArray(bufferIndex);
}

GLElementBuffer::GLElementBuffer() {
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BASE_CAPACITY * sizeof(elemType), nullptr, GL_DYNAMIC_DRAW);
}

void GLElementBuffer::write(const std::vector<elemType> &data) {
    size = data.size();
    updateBufferCapacity(size);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size * sizeof(elemType), data.data());
}

void GLElementBuffer::enable() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
}

// template specializations

template class GLBuffer<glm::uvec3>;
template class GLBuffer<glm::ivec3>;
template class GLBuffer<glm::vec3>;
template class GLBuffer<glm::vec2>;
template class GLBuffer<glm::uint32>;
template class GLBuffer<unsigned short>;
template class GLBuffer<int>;

template class GLArrayBuffer<glm::uvec3>;
template class GLArrayBuffer<glm::ivec3>;
template class GLArrayBuffer<glm::vec3>;
template class GLArrayBuffer<glm::vec2>;
template class GLArrayBuffer<glm::uint32>;
template class GLArrayBuffer<int>;
