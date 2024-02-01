#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

#include <cstdint>
#include <stdexcept>
#include <array>
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
    FaceMapping(Args &&... args) : mapping({ args... }) {}

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

// this is purposefully not an enum class, as we want to use the underlying numeric values
// to also index into texture samplers inside the cube fragment shader
enum EBlockType : std::uint8_t {
    BlockType_None = 0,
    BlockType_Grass,
    BlockType_Dirt,
    BlockType_Stone,
    BlockType_NumTypes
};

class Block {
public:
    /**
     * a single block's width.
     * this is NOT intended be changed, this it can (and will) break if changed to any other value than `1.0f`.
     * this is the case because we'd like to preserve the 1-1 mapping between (floored) world coordinates and blocks
     */
    static constexpr float RENDER_SIZE = 1.0f;

    EBlockType blockType = EBlockType::BlockType_Grass;

    /*
        the below offsets signify a standardized specific ordering of vertices of a cube,
        used primarily in the mesh constructing algorithm and the face merging algorithm.

        the vertices are numbered as follows:
           6--------7
          /|       /|
         / |      / |
        3--------2  |
        |  |     |  |
        |  5-----|--4
        | /      | /
        |/       |/
        0--------1
        where the front face is the 0-1-2-3 one.
    */
    static constexpr std::array<glm::vec3, 8> vertexOffsets = {
        {
            Block::RENDER_SIZE * glm::vec3(-0.5, -0.5, 0.5),  // 0
            Block::RENDER_SIZE * glm::vec3(0.5, -0.5, 0.5),   // 1
            Block::RENDER_SIZE * glm::vec3(0.5, 0.5, 0.5),    // 2
            Block::RENDER_SIZE * glm::vec3(-0.5, 0.5, 0.5),   // 3
            Block::RENDER_SIZE * glm::vec3(0.5, -0.5, -0.5),  // 4
            Block::RENDER_SIZE * glm::vec3(-0.5, -0.5, -0.5), // 5
            Block::RENDER_SIZE * glm::vec3(-0.5, 0.5, -0.5),  // 6
            Block::RENDER_SIZE * glm::vec3(0.5, 0.5, -0.5)    // 7
        }
    };

public:
    Block() = default;

    explicit Block(const EBlockType type) : blockType(type) {}

    [[nodiscard]]
    bool isNone() const { return blockType == EBlockType::BlockType_None; }

    /**
     * @param face A cube face.
     * @return The bottom-left and top-right corners of a given face.
     * This does not return vertices with lowest and highest coordinates respectively,
     * but it considers them "as the face is looked at by an observer" if that makes sense.
     */
    static std::pair<glm::vec3, glm::vec3> getFaceCorners(const EBlockFace face) {
        switch (face) {
            case Front:
                return {vertexOffsets[0], vertexOffsets[2]};
            case Back:
                return {vertexOffsets[4], vertexOffsets[6]};
            case Right:
                return {vertexOffsets[1], vertexOffsets[7]};
            case Left:
                return {vertexOffsets[5], vertexOffsets[3]};
            case Top:
                return {vertexOffsets[3], vertexOffsets[7]};
            case Bottom:
                return {vertexOffsets[5], vertexOffsets[1]};
            default:
                throw std::runtime_error("invalid face value in getFaceCorners()");
        }
    }
};

#endif //MYGE_BLOCK_H
