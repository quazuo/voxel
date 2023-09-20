#ifndef VOXEL_TEXTURE_MANAGER_H
#define VOXEL_TEXTURE_MANAGER_H

#include "GL/glew.h"
#include "src/voxel/block.h"
#include <map>
#include <filesystem>

class TextureManager {
    using TextureMap = std::map<std::pair<EBlockType, EBlockFace>, GLuint>;
    TextureMap blockTextures;

    GLuint fontTexture;

public:
    void loadBlockTexture(EBlockType blockType, std::uint8_t faces, const std::filesystem::path& path);

    void loadFontTexture(const std::filesystem::path& path);

    void bindBlockTextures(GLuint blockShaderID) const;

    void bindFontTexture(GLuint textShaderID) const;

    [[nodiscard]]
    GLuint getBlockTextureID(EBlockType blockType, EBlockFace face) const;

    [[nodiscard]]
    static int getBlockSamplerID(EBlockType blockType, EBlockFace face);

    [[nodiscard]]
    GLuint getFontTextureID() const { return fontTexture; }

private:
    static GLuint loadDDS(const std::filesystem::path& path);
};

#endif //VOXEL_TEXTURE_MANAGER_H
