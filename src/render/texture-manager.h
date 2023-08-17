#ifndef VOXEL_TEXTURE_MANAGER_H
#define VOXEL_TEXTURE_MANAGER_H

#include "GL/glew.h"
#include "src/voxel/block.h"
#include <map>
#include <filesystem>

class TextureManager {
    std::map<EBlockType, GLuint> blockTextures;

    GLuint fontTexture;

public:
    void loadBlockTexture(EBlockType tex, const std::filesystem::path& path);

    void loadFontTexture(const std::filesystem::path& path);

    void bindBlockTextures(GLuint blockShaderID) const;

    void bindFontTexture(GLuint textShaderID) const;

    [[nodiscard]]
    GLuint getBlockTextureID(EBlockType tex) const;

    [[nodiscard]]
    GLuint getFontTextureID() const { return fontTexture; }

private:
    static GLuint loadDDS(const std::filesystem::path& path);
};

#endif //VOXEL_TEXTURE_MANAGER_H
