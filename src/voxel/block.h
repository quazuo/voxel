#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

#include "src/utils/vec.h"

enum EBlockType : std::uint8_t {
    BlockType_None,
    BlockType_Grass,
    BlockType_Dirt,
    BlockType_NumTypes
};

class Block {
public:
    Block() = default;

    explicit Block(EBlockType type) : blockType(type) {}

    EBlockType blockType = EBlockType::BlockType_Grass;

    static constexpr double RENDER_SIZE = .5;

    [[nodiscard]]
    bool isNone() const {
        return blockType == EBlockType::BlockType_None;
    }
};

#endif //MYGE_BLOCK_H
