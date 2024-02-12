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

GLFrameBuffer::GLFrameBuffer() {
    glGenFramebuffers(1, &bufferID);
}

GLFrameBuffer::~GLFrameBuffer() {
    glDeleteFramebuffers(1, &bufferID);
}

void GLFrameBuffer::enable() const {
    glBindFramebuffer(GL_FRAMEBUFFER, bufferID);
}

void GLFrameBuffer::disable() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::attachTexture(const glm::ivec2 size) {
    enable();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    disable();
}

void GLFrameBuffer::attachDepth(const glm::ivec2 size) {
    glGenTextures(1, &depth);
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    enable();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    disable();
}

void GLFrameBuffer::attachDepthStencil(const glm::ivec2 size) {
    enable();

    glGenTextures(1, &depthStencil);
    glBindTexture(GL_TEXTURE_2D, depthStencil);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.x, size.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencil, 0);

    disable();
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
