#ifndef VOXEL_TEXTURE_MANAGER_H
#define VOXEL_TEXTURE_MANAGER_H

#include "GL/glew.h"
#include "src/voxel/block.h"
#include <map>
#include <filesystem>

class TextureManager {
    std::map<EBlockType, GLuint> loadedTextures;

    int nextTextureUnit = 0;

public:
    void loadTexture(EBlockType tex, const std::filesystem::path& path);

    void bindTextures(GLuint programID);

    [[nodiscard]]
    GLuint getTextureID(EBlockType tex) const;
};

#endif //VOXEL_TEXTURE_MANAGER_H
