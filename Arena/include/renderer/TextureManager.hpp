#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <unordered_map>

class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    bool initialize();
    GLuint getTextureID(int index) const;
    void bindTexture(int index);

    // Texture loading
    bool loadBlockTextures();
    GLuint getBlockTextureArray() const { return m_blockTextureArray; }
    
    // Block texture indices
    static constexpr int COBBLESTONE_TEXTURE = 0;
    static constexpr int DIRT_TEXTURE = 1;
    static constexpr int GRASS_TEXTURE = 2;
    static constexpr int TEXTURE_SIZE = 16;
    static constexpr int NUM_TEXTURES = 3;

private:
    bool loadTexture(const std::string& filename, GLuint& textureID);
    bool loadTextureToArray(const std::string& path, int layer);
    void cleanup();

    GLuint m_blockTextureArray;  // 2D texture array for all block textures
    std::vector<GLuint> m_textureIDs;
    static const std::string TEXTURE_PATHS[NUM_TEXTURES];
}; 