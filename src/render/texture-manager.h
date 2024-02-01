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

    GLuint skyboxCubemap{};

    GLuint fontTexture{};

public:
    using BlockTexPathMapping = const std::map<EBlockType, FaceMapping<std::filesystem::path>>;

    /**
     * Loads all given block textures at given paths and applies them to all faces of given block types.
     *
     * @param blockTexPathMappings Map containing paths to texture for each face of each block.
     */
    void loadBlockTextures(const BlockTexPathMapping& blockTexPathMappings);

    /**
     * Loads a texture at a given path and applies it to the skybox.
     *
     * @param skyboxTexturePaths Paths to files containing the textures.
     */
    void loadSkyboxTextures(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);

    /**
     * Loads a texture at a given path and applies it to all rendered text.
     * TODO: this will be removed when ImGUI will be incorporated into the project.
     *
     * @param path Path to a file containing the texture.
     */
    void loadFontTexture(const std::filesystem::path &path);

    /**
     * Binds all managed block textures so that they can be used by the provided shader.
     *
     * @param blockShaderID ID of the shader program which will use all the textures.
     */
    void bindBlockTextures(GLuint blockShaderID) const;

    /**
     * Binds all managed skybox textures so that they can be used by the provided shader.
     *
     * @param skyboxShaderID ID of the shader program which will use all the textures.
     */
    void bindSkyboxTextures(GLuint skyboxShaderID) const;

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
     * Loads a texture stored inside a given file.
     *
     * @param path Path to the file.
     * @return OpenGL handle to the newly loaded texture.
     */
    static GLuint loadTexture(const std::filesystem::path& path);

    /**
     * Loads a cubemap texture stored inside a given file.
     *
     * @param skyboxTexturePaths Paths to texture files.
     * @return OpenGL handle to the newly loaded cubemap texture.
     */
    static GLuint loadCubemapTexture(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);

    /**
     * Loads a texture stored inside a given .DDS file.
     *
     * @param path Path to the file.
     * @return OpenGL handle to the newly loaded texture.
     */
    static GLuint loadTextureDDS(const std::filesystem::path &path);

    /**
     * Loads a cubemap texture stored inside a given .DDS file.
     *
     * @param skyboxTexturePaths Paths to texture files.
     * @return OpenGL handle to the newly loaded cubemap texture.
     */
    static GLuint loadCubemapDDS(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);
};

#endif //VOXEL_TEXTURE_MANAGER_H
