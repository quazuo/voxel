#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

#include <cstdint>

// this is *purposefully* not an enum class, as we want to use the underlying numeric values
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

    // a block's width.
    // this *should not* be changed, as we'd like to preserve the 1-1 mapping
    // between (floored) world coordinates and blocks
    static constexpr float RENDER_SIZE = 1.0f;

    [[nodiscard]]
    bool isNone() const {
        return blockType == EBlockType::BlockType_None;
    }
};

#endif //MYGE_BLOCK_H
