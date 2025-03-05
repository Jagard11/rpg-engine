// ./include/Graphics/ResourceManager.hpp
#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

// Include GLEW first
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "../../third_party/stb/stb_image.h" // Include stb_image directly

class ResourceManager {
public:
    // Singleton instance accessor
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }
    
    // Load a texture from file
    GLuint loadTexture(const std::string& filename) {
        // Check if already loaded
        auto it = textures.find(filename);
        if (it != textures.end()) {
            return it->second;
        }
        
        // Create and load new texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        // Load image data
        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        
        if (data) {
            // Upload to GPU
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                         channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
            
            std::cout << "Texture loaded: " << filename << " (" << width << "x" << height 
                      << ", channels: " << channels << ")" << std::endl;
                      
            // Store in map
            textures[filename] = textureID;
            
            // Free image data
            stbi_image_free(data);
            return textureID;
        } else {
            std::cerr << "Failed to load texture: " << filename << " - " 
                      << stbi_failure_reason() << std::endl;
            
            // Create a fallback texture (checkerboard pattern)
            createFallbackTexture(textureID);
            textures[filename] = textureID;
            
            stbi_image_free(data);
            return textureID;
        }
    }
    
    // Create and get fallback texture
    GLuint getFallbackTexture() {
        if (fallbackTexture == 0) {
            glGenTextures(1, &fallbackTexture);
            createFallbackTexture(fallbackTexture);
        }
        return fallbackTexture;
    }
    
    // Named texture getter
    GLuint getTexture(const std::string& name) {
        auto it = textures.find(name);
        if (it != textures.end()) {
            return it->second;
        }
        return getFallbackTexture();
    }
    
    // Cleanup all resources
    void cleanup() {
        for (auto& pair : textures) {
            glDeleteTextures(1, &pair.second);
        }
        textures.clear();
        
        if (fallbackTexture != 0) {
            glDeleteTextures(1, &fallbackTexture);
            fallbackTexture = 0;
        }
    }

private:
    // Private constructor for singleton
    ResourceManager() : fallbackTexture(0) {}
    
    // Prevent copying
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Create a checkerboard fallback texture
    void createFallbackTexture(GLuint textureID) {
        const int checkerSize = 64;
        const int textureSize = 256;
        std::vector<unsigned char> checkerboard(textureSize * textureSize * 4);
        
        for (int y = 0; y < textureSize; y++) {
            for (int x = 0; x < textureSize; x++) {
                int idx = (y * textureSize + x) * 4;
                bool isGreen = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
                
                if (isGreen) {
                    // Green (for grass)
                    checkerboard[idx + 0] = 34;     // R
                    checkerboard[idx + 1] = 139;    // G
                    checkerboard[idx + 2] = 34;     // B
                } else {
                    // Brown (for dirt)
                    checkerboard[idx + 0] = 139;    // R
                    checkerboard[idx + 1] = 69;     // G
                    checkerboard[idx + 2] = 19;     // B
                }
                checkerboard[idx + 3] = 255;  // A (fully opaque)
            }
        }
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, 
                     GL_UNSIGNED_BYTE, checkerboard.data());
                     
        std::cout << "Created fallback texture: " << textureSize << "x" << textureSize << std::endl;
    }
    
    // Storage
    std::unordered_map<std::string, GLuint> textures;
    GLuint fallbackTexture;
};

#endif // RESOURCE_MANAGER_HPP