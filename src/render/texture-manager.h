#ifndef VOXEL_TEXTURE_MANAGER_H
#define VOXEL_TEXTURE_MANAGER_H

#include "GL/glew.h"
#include "src/voxel/block.h"
#include <map>
#include <filesystem>

/**
 * Class managing all the textures used by the renderer.
 * This currently includes only textures for blocks as well as for text.
 *
 * TODO: fontTexture as well as text support as a whole will be removed
 * TODO: when ImGUI will be incorporated into the project.
 */
class TextureManager {
    using TextureMap = std::map<std::pair<EBlockType, EBlockFace>, GLuint>;
    TextureMap blockTextures;

    GLuint fontTexture{};

public:
    /**
     * Loads a texture at a given path and applies it to certain faces of a block type.
     *
     * @param blockType Type of block which should use the texture.
     * @param faces Block faces which should use the texture.
     * @param path Path to a file containing the texture.
     */
    void loadBlockTexture(EBlockType blockType, std::uint8_t faces, const std::filesystem::path& path);

    /**
     * Loads a texture at a given path and applies it to all rendered text.
     * TODO: this will be removed when ImGUI will be incorporated into the project.
     *
     * @param path Path to a file containing the texture.
     */
    void loadFontTexture(const std::filesystem::path& path);

    /**
     * Binds all managed block textures so that they can be used by the provided shader.
     *
     * @param blockShaderID ID of the shader program which will use all the textures.
     */
    void bindBlockTextures(GLuint blockShaderID) const;

    /**
     * Binds the font texture so that it can be used by the provided shader.
     *
     * @param textShaderID ID of the shader program which will use the font texture.
     */
    void bindFontTexture(GLuint textShaderID) const;

    [[nodiscard]]
    GLuint getBlockTextureID(EBlockType blockType, EBlockFace face) const;

    [[nodiscard]]
    static int getBlockSamplerID(EBlockType blockType, EBlockFace face);

    [[nodiscard]]
    GLuint getFontTextureID() const { return fontTexture; }

private:
    /**
     * Loads a texture inside a .DDS file.
     *
     * @param path Path to the file.
     * @return OpenGL handle to the newly loaded texture.
     */
    static GLuint loadDDS(const std::filesystem::path& path);
};

#endif //VOXEL_TEXTURE_MANAGER_H
