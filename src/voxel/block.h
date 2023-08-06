#ifndef MYGE_BLOCK_H
#define MYGE_BLOCK_H

enum class EBlockType {
    BlockType_None = 0,
    BlockType_Stone,
    BlockType_Grass,
    BlockType_NumTypes
};

class Block {
public:
    Block() = default;

    explicit Block(EBlockType type) : blockType(type) {}

    EBlockType blockType = EBlockType::BlockType_Stone;

    static constexpr double RENDER_SIZE = .5;

    [[nodiscard]]
    bool isNone() const {
        return blockType == EBlockType::BlockType_None;
    }
};

#endif //MYGE_BLOCK_H
