#include "gl-buffer.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

template<typename T>
void GLBuffer<T>::free() {
    const GLuint buffers[1] = { bufferID };
    glDeleteBuffers(1, buffers);
}

template<typename T>
void GLBuffer<T>::updateBufferCapacity(GLsizeiptr dataSize) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);

    if (dataSize > capacity) {
        // buffer too small, need to expand it
        GLsizeiptr newCapacity = capacity;

        while (dataSize > newCapacity) {
            newCapacity *= 2;
        }

        capacity = newCapacity;
        glBufferData(GL_ARRAY_BUFFER, newCapacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);

    } else if (dataSize <= capacity / 4) {
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
void GLArrayBuffer<T>::init(GLuint index, GLint count) {
    if (this->isInit) return;

    this->isInit = true;
    bufferIndex = index;
    compCount = count;

    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glVertexAttribPointer(bufferIndex, compCount, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, this->BASE_CAPACITY * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
}

template<typename T>
void GLArrayBuffer<T>::write(const std::vector<T> &data) {
    size_t dataSize = data.size();

    this->updateBufferCapacity(dataSize);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize * sizeof(T), &data[0]);
}

template<typename T>
void GLArrayBuffer<T>::enable() {
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glVertexAttribPointer(bufferIndex, compCount, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(bufferIndex);
}

template<typename T>
void GLArrayBuffer<T>::disable() {
    glDisableVertexAttribArray(bufferIndex);
}

void GLElementBuffer::init() {
    if (isInit) return;

    isInit = true;

    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BASE_CAPACITY * sizeof(elemType), nullptr, GL_DYNAMIC_DRAW);
}

void GLElementBuffer::write(const std::vector<elemType> &data) {
    size_t dataSize = data.size();

    updateBufferCapacity(dataSize);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, dataSize * sizeof(elemType), &data[0]);
}

void GLElementBuffer::enable() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
}

// template specializations

template class GLBuffer<glm::vec3>;
template class GLBuffer<glm::vec2>;
template class GLBuffer<unsigned short>;

template class GLArrayBuffer<glm::vec3>;
template class GLArrayBuffer<glm::vec2>;
