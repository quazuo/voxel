#ifndef FACE_H
#define FACE_H

#include <cstdint>
#include <stdexcept>
#include <map>
#include "glm/vec3.hpp"

enum EBlockFace : std::uint8_t {
    Front = 1 << 0,
    Back = 1 << 1,
    Right = 1 << 2,
    Left = 1 << 3,
    Top = 1 << 4,
    Bottom = 1 << 5,
    N_FACES = 6
};

static constexpr std::uint8_t ALL_FACES = Front | Back | Right | Left | Top | Bottom;
static constexpr std::uint8_t ALL_SIDE_FACES = Front | Back | Right | Left;

/**
 * Utility class for various mappings between faces and other kinds of things.
 * Made mostly as a wrapper over std::map, but saving a bit of memory when using
 * same values across all faces or all side faces.
 */
template<typename T>
class FaceMapping {
    std::map<uint8_t, T> mapping;

public:
    template<typename ...Args>
    explicit FaceMapping(Args &&... args) : mapping({ args... }) {}

    void insert(const uint8_t key, const T& value) {
        switch (key) {
            case Front:
            case Back:
            case Right:
            case Left:
            case Top:
            case Bottom:
            case ALL_FACES:
            case ALL_SIDE_FACES:
                break;
            default:
                throw std::runtime_error("invalid key value in FaceMapping::insert");
        }

        mapping.emplace(key, value);
    }

    [[nodiscard]]
    bool contains(const uint8_t key) const {
        return mapping.contains(key);
    }

    [[nodiscard]]
    T get(const EBlockFace key) const {
        if (key == N_FACES) {
            throw std::runtime_error("invalid key value in FaceMapping::get");
        }

        if (mapping.contains(ALL_FACES)) {
            return mapping.at(ALL_FACES);
        }

        switch (key) {
            case Front:
            case Back:
            case Right:
            case Left:
                if (mapping.contains(ALL_SIDE_FACES)) {
                    return mapping.at(ALL_SIDE_FACES);
                }
            case Top:
            case Bottom:
                return mapping.at(key);
            default: // to silence warnings
                throw std::runtime_error("invalid key value in FaceMapping::get");
        }
    }
};

static glm::vec3 getNormalFromFace(const EBlockFace face) {
    switch (face) {
        case Front:
            return {0, 0, 1};
        case Back:
            return {0, 0, -1};
        case Right:
            return {1, 0, 0};
        case Left:
            return {-1, 0, 0};
        case Top:
            return {0, 1, 0};
        case Bottom:
            return {0, -1, 0};
        default:
            throw std::runtime_error("invalid normal in getFaceFromNormal()");
    }
}

static EBlockFace getFaceFromNormal(const glm::vec3& normal) {
    if (normal == glm::vec3(0, 0, 1))
        return Front;
    if (normal == glm::vec3(0, 0, -1))
        return Back;
    if (normal == glm::vec3(1, 0, 0))
        return Right;
    if (normal == glm::vec3(-1, 0, 0))
        return Left;
    if (normal == glm::vec3(0, 1, 0))
        return Top;
    if (normal == glm::vec3(0, -1, 0))
        return Bottom;
    throw std::runtime_error("invalid normal in getFaceFromNormal()");
}

#endif //FACE_H
