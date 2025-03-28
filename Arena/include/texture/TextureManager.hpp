#pragma once

#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include <glm/glm.hpp>

class TextureManager {
public:
    static TextureManager& getInstance();
    
    // Load a texture from a file
    GLuint loadTexture(const std::string& filename);
    
    // Get a previously loaded texture
    GLuint getTexture(const std::string& filename) const;
    
    // Bind a texture to a specific texture unit
    void bindTexture(GLuint textureId, GLenum textureUnit = GL_TEXTURE0);
    
    // Clean up all loaded textures
    void cleanup();

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    std::unordered_map<std::string, GLuint> m_textures;
}; 