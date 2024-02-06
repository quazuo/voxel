#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

#include <cstdint>
#include <stdexcept>
#include <array>
#include <map>
#include "glm/vec3.hpp"
#include "face.h"
#include "src/utils/vec.h"

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
    static constexpr std::array<glm::ivec3, 8> vertexOffsets = {
        {
            { 0, 0, 1 }, // 0
            { 1, 0, 1 }, // 1
            { 1, 1, 1 }, // 2
            { 0, 1, 1 }, // 3
            { 1, 0, 0 }, // 4
            { 0, 0, 0 }, // 5
            { 0, 1, 0 }, // 6
            { 1, 1, 0 }, // 7
        }
    };

    Block() = default;

    explicit Block(const EBlockType type) : blockType(type) {
    }

    [[nodiscard]]
    bool isNone() const { return blockType == EBlockType::BlockType_None; }

    /**
     * @param face A cube face.
     * @return The bottom-left and top-right corners of a given face.
     * This does not return vertices with lowest and highest coordinates respectively,
     * but it considers them "as the face is looked at by an observer" if that makes sense.
     */
    static std::pair<glm::ivec3, glm::ivec3> getFaceCorners(const EBlockFace face) {
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
