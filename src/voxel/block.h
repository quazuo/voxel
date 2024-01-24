#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

#include <cstdint>
#include <stdexcept>
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

static inline glm::vec3 getNormalFromFace(EBlockFace face) {
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

static inline EBlockFace getFaceFromNormal(glm::vec3 normal) {
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

static constexpr std::uint8_t ALL_FACES = Front | Back | Right | Left | Top | Bottom;
static constexpr std::uint8_t ALL_SIDE_FACES = Front | Back | Right | Left;

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
    Block() = default;

    explicit Block(EBlockType type) : blockType(type) {}

    EBlockType blockType = EBlockType::BlockType_Grass;

    /**
     * a single block's width.
     * this is NOT intended be changed, this it can (and will) break if changed to any other value than `1.0f`.
     * this is the case because we'd like to preserve the 1-1 mapping between (floored) world coordinates and blocks
     */
    static constexpr float RENDER_SIZE = 1.0f;

    [[nodiscard]]
    bool isNone() const {
        return blockType == EBlockType::BlockType_None;
    }
};

#endif //MYGE_BLOCK_H
