#include "debug/DebugMenu.hpp"
#include "core/Game.hpp"
#include "core/StackTrace.hpp"
#include "debug/DebugUtils.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cctype>
#include <map>
#include <memory>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Debug {

TextRenderer::TextRenderer() : VAO(0), VBO(0), shaderProgram(0), ft(nullptr), face(nullptr) {
    std::cout << "Initializing TextRenderer..." << std::endl;
    
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }
    std::cout << "FreeType initialized successfully" << std::endl;

    // Try to load system fonts in order of preference
    const char* fontPaths[] = {
        // Windows paths (prioritized)
        "C:\\Windows\\Fonts\\impact.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
        // Linux paths
        "/usr/share/fonts/truetype/msttcorefonts/impact.ttf",
        "/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
        "/usr/share/fonts/truetype/msttcorefonts/arialbd.ttf",
        // macOS paths
        "/Library/Fonts/Impact.ttf",
        "/Library/Fonts/Arial.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        // Fallback Linux paths
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    };

    bool fontLoaded = false;
    for (const char* fontPath : fontPaths) {
        std::cout << "Attempting to load font: " << fontPath << std::endl;
        if (FT_New_Face(ft, fontPath, 0, &face) == 0) {
            std::cout << "Successfully loaded font: " << fontPath << std::endl;
            fontLoaded = true;
            break;
        } else {
            std::cerr << "Failed to load font: " << fontPath << " (error code: " 
                      << FT_Err_Ok << ")" << std::endl;
        }
    }

    if (!fontLoaded) {
        std::cerr << "ERROR::FREETYPE: Failed to load any system font. Text rendering will be disabled." << std::endl;
        FT_Done_FreeType(ft);
        ft = nullptr;
        return;
    }

    // Set larger font size for better readability
    FT_Set_Pixel_Sizes(face, 0, 48);  // Increased from 32 to 48
    std::cout << "Set font pixel size to 48" << std::endl;

    // Create and compile the shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
        out vec2 TexCoords;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;

        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    // Create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        std::cerr << "ERROR: Failed to create vertex shader" << std::endl;
        return;
    }
    std::cout << "Created vertex shader: " << vertexShader << std::endl;
    
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    std::cout << "Vertex shader compiled successfully" << std::endl;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        std::cerr << "ERROR: Failed to create fragment shader" << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    std::cout << "Created fragment shader: " << fragmentShader << std::endl;
    
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    std::cout << "Fragment shader compiled successfully" << std::endl;

    // Create shader program
    shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        std::cerr << "ERROR: Failed to create shader program" << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    std::cout << "Created shader program: " << shaderProgram << std::endl;
    
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check program linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return;
    }
    std::cout << "Shader program linked successfully" << std::endl;

    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    textUniformLoc = glGetUniformLocation(shaderProgram, "text");
    textColorUniformLoc = glGetUniformLocation(shaderProgram, "textColor");
    projectionUniformLoc = glGetUniformLocation(shaderProgram, "projection");

    if (textUniformLoc == -1 || textColorUniformLoc == -1 || projectionUniformLoc == -1) {
        std::cerr << "ERROR: Failed to get shader uniform locations" << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return;
    }
    std::cout << "Got uniform locations: text=" << textUniformLoc 
              << ", textColor=" << textColorUniformLoc 
              << ", projection=" << projectionUniformLoc << std::endl;

    // Set default uniform values
    glUseProgram(shaderProgram);
    glUniform1i(textUniformLoc, 0); // Set text sampler to texture unit 0
    glUniform3f(textColorUniformLoc, 1.0f, 1.0f, 1.0f); // Default white color
    glUseProgram(0);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Allocate buffer with enough space for text rendering
    // Each character needs 6 vertices, each vertex has 4 floats (pos + tex)
    const GLsizeiptr bufferSize = sizeof(GLfloat) * 6 * 4 * 128;  // Space for 128 characters
    glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
    
    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to allocate vertex buffer. Error code: " << err << std::endl;
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
        VAO = VBO = 0;
        return;
    }
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize projection matrix with default values
    projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
    std::cout << "Initialized projection matrix" << std::endl;

    // Load all ASCII characters
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction
    int loadedChars = 0;
    for (GLubyte c = 0; c < 128; c++) {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph: " << c << std::endl;
            continue;
        }

        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Check for texture creation errors
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "ERROR: Failed to create texture for character '" << c << "'. Error code: " << err << std::endl;
            glDeleteTextures(1, &texture);
            continue;
        }

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Check for texture parameter errors
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "ERROR: Failed to set texture parameters for character '" << c << "'. Error code: " << err << std::endl;
            glDeleteTextures(1, &texture);
            continue;
        }

        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
        loadedChars++;
    }

    std::cout << "TextRenderer initialized successfully with " << loadedChars << " characters" << std::endl;

    // Check if initialization was successful
    if (!shaderProgram || !VAO || !VBO || Characters.empty()) {
        std::cerr << "ERROR: TextRenderer initialization failed!" << std::endl;
        return;
    }

    std::cout << "TextRenderer initialized successfully" << std::endl;
}

TextRenderer::~TextRenderer() {
    // Cleanup FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Cleanup OpenGL resources
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
    // Delete all character textures
    for (auto& ch : Characters) {
        glDeleteTextures(1, &ch.second.TextureID);
    }
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    if (!shaderProgram || !VAO || !VBO) {
        std::cerr << "ERROR: Text renderer not properly initialized (shaderProgram=" 
                  << shaderProgram << ", VAO=" << VAO << ", VBO=" << VBO << ")" << std::endl;
        renderFallbackText(text, x, y, scale, color);
        return;
    }

    if (Characters.empty()) {
        std::cerr << "ERROR: No font characters loaded" << std::endl;
        renderFallbackText(text, x, y, scale, color);
        return;
    }

    std::cout << "Rendering text: '" << text << "' at (" << x << "," << y << ") with scale " << scale << std::endl;

    // Save current OpenGL state
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    GLint currentActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);
    GLint currentTextureBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTextureBinding);

    // Activate corresponding render state	
    glUseProgram(shaderProgram);
    
    // Set uniforms using stored locations
    if (textColorUniformLoc == -1 || projectionUniformLoc == -1) {
        std::cerr << "ERROR: Invalid uniform locations (textColor=" << textColorUniformLoc 
                << ", projection=" << projectionUniformLoc << ")" << std::endl;
        glUseProgram(currentProgram);
        renderFallbackText(text, x, y, scale, color);
        return;
    }
    
    // Set up rendering state
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    
    // Update projection and color uniforms
    glUniform3f(textColorUniformLoc, color.x, color.y, color.z);
    glUniformMatrix4fv(projectionUniformLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Pre-allocate vertex data buffer for text rendering
    std::vector<GLfloat> vertices;
    vertices.reserve(text.length() * 6 * 4);  // 6 vertices per char, 4 floats per vertex
    
    // Track if we need to fall back to simpler rendering method
    bool renderingFailed = false;

    // Iterate through all characters
    for (auto c = text.begin(); c != text.end(); c++) {
        auto it = Characters.find(*c);
        if (it == Characters.end()) continue;
        
        Character ch = it->second;

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Add vertices for this character
        vertices.insert(vertices.end(), {
            xpos,     ypos + h, 0.0f, 0.0f,            
            xpos,     ypos,     0.0f, 1.0f,
            xpos + w, ypos,     1.0f, 1.0f,

            xpos,     ypos + h, 0.0f, 0.0f,
            xpos + w, ypos,     1.0f, 1.0f,
            xpos + w, ypos + h, 1.0f, 0.0f           
        });

        // Bind texture and update buffer
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        // Update the buffer for this character
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat), vertices.data()); 
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error during buffer update: " << err << std::endl;
            renderingFailed = true;
            break;
        }
        
        // Draw the character
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 4);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error during drawing: " << err << std::endl;
            renderingFailed = true;
            break;
        }

        // Now advance cursors for next glyph
        x += (ch.Advance >> 6) * scale;
    }

    // Restore OpenGL state
    glBindVertexArray(currentVAO);
    glActiveTexture(currentActiveTexture);
    glBindTexture(GL_TEXTURE_2D, currentTextureBinding);
    glUseProgram(currentProgram);
    
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    
    // If we had rendering failures, try the fallback method
    if (renderingFailed) {
        renderFallbackText(text, x, y, scale, color);
    }
}

void TextRenderer::renderFallbackText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    std::cout << "Using fallback text rendering for: " << text << std::endl;
    
    // Save previous state
    GLfloat prevColor[4];
    glGetFloatv(GL_CURRENT_COLOR, prevColor);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrcAlpha, prevBlendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDstAlpha);
    
    // Setup for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use immediate mode for fallback
    glUseProgram(0);
    
    // Doubled character size for better readability
    float charWidth = 24.0f * scale;   // Doubled from 12.0f
    float charHeight = 40.0f * scale;  // Doubled from 20.0f
    float xpos = x;
    
    // First draw a semi-transparent background for the text
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Dark background with more opacity
    glBegin(GL_QUADS);
    glVertex2f(x - 8, y - 8);
    glVertex2f(x + text.length() * charWidth + 8, y - 8);
    glVertex2f(x + text.length() * charWidth + 8, y + charHeight + 8);
    glVertex2f(x - 8, y + charHeight + 8);
    glEnd();
    
    // Draw a border around the text background
    glColor4f(0.5f, 0.5f, 1.0f, 1.0f); // Light blue border
    glLineWidth(3.0f); // Thicker line for better visibility
    glBegin(GL_LINE_LOOP);
    glVertex2f(x - 8, y - 8);
    glVertex2f(x + text.length() * charWidth + 8, y - 8);
    glVertex2f(x + text.length() * charWidth + 8, y + charHeight + 8);
    glVertex2f(x - 8, y + charHeight + 8);
    glEnd();
    glLineWidth(1.0f); // Reset line width
    
    // Now draw each character
    for (unsigned int i = 0; i < text.length(); i++) {
        char c = text[i];
        
        // Check for lowercase and convert to uppercase
        if (c >= 'a' && c <= 'z') {
            c = std::toupper(c);
        }
        
        // Choose a color based on character type
        if (c >= 'A' && c <= 'Z') {
            glColor3f(1.0f, 1.0f, 1.0f); // White for letters
        } else if (c >= '0' && c <= '9') {
            glColor3f(1.0f, 0.8f, 0.2f); // Gold for numbers
        } else if (c == '>') {
            glColor3f(0.0f, 1.0f, 0.0f); // Green for command prompt
        } else if (c == '_') {
            glColor3f(1.0f, 1.0f, 0.0f); // Yellow for cursor
        } else {
            glColor3f(1.0f, 1.0f, 0.0f); // Yellow for other special characters
        }
        
        // Draw character background
        glBegin(GL_QUADS);
        glVertex2f(xpos, y);
        glVertex2f(xpos + charWidth, y);
        glVertex2f(xpos + charWidth, y + charHeight);
        glVertex2f(xpos, y + charHeight);
        glEnd();
        
        // Draw character outline for better visibility
        glColor3f(0.3f, 0.3f, 0.3f); // Dark outline
        glBegin(GL_LINE_LOOP);
        glVertex2f(xpos, y);
        glVertex2f(xpos + charWidth, y);
        glVertex2f(xpos + charWidth, y + charHeight);
        glVertex2f(xpos, y + charHeight);
        glEnd();
        
        // Draw the actual character
        glBegin(GL_LINES);
        switch(c) {
            // Basic Latin uppercase
            case 'A':
                // Draw an 'A' shape
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                break;
            case 'B':
                // Draw a 'B' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                break;
            case 'C':
                // Draw a 'C' shape
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                break;
            case 'D':
                // Draw a 'D' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                break;
            case 'E':
                // Draw an 'E' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case 'F':
                // Draw an 'F' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.5f);
                break;
            case 'G':
                // Draw a 'G' shape
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                break;
            case 'H':
                // Draw an 'H' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                break;
            case 'I':
                // Draw an 'I' shape
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case 'J':
                // Draw a 'J' shape
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                break;
            case 'K':
                // Draw a 'K' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case 'L':
                // Draw an 'L' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case 'M':
                // Draw an 'M' shape
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                break;
            case 'N':
                // Draw an 'N' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                break;
            case 'O':
                // Draw an 'O' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                break;
            case 'P':
                // Draw a 'P' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                break;
            case 'Q':
                // Draw a 'Q' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                break;
            case 'R':
                // Draw an 'R' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case 'S':
                // Draw an 'S' shape
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                break;
            case 'T':
                // Draw a 'T' shape
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                break;
            case 'U':
                // Draw a 'U' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                break;
            case 'V':
                // Draw a 'V' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                break;
            case 'W':
                // Draw a 'W' shape
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.8f);
                break;
            case 'X':
                // Draw an 'X' shape - fixed
                glColor3f(1.0f, 0.0f, 0.0f); // Make X red for visibility
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                break;
            case 'Y':
                // Draw a 'Y' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                break;
            case 'Z':
                // Draw a 'Z' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            // Numbers
            case '0':
                // Draw a '0' shape (similar to 'O')
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                break;
            case '1':
                // Draw a '1' shape
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.8f);
                break;
            case '2':
                // Draw a '2' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.2f);
                break;
            case '8':
                // Draw an '8' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.7f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                break;
            // Special characters
            case ' ':
                // Space - just skip
                break;
            case '.':
                // Draw a period
                glBegin(GL_POINTS);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight * 0.2f);
                glEnd();
                break;
            case '>':
                // Draw a '>' shape
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.3f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.7f);
                break;
            case '_':
                // Draw an underscore for the cursor
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                break;
            case '(':
                // Draw a '(' shape
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.8f);
                break;
            case ')':
                // Draw a ')' shape
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.6f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.4f, y + charHeight * 0.8f);
                break;
            default:
                // For other characters, just draw a simple line
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                break;
        }
        glEnd();
        
        xpos += charWidth;
    }
    
    // Restore previous state
    glColor4fv(prevColor);
    if (!prevBlend) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(prevBlendSrcAlpha, prevBlendDstAlpha);
}

void TextRenderer::updateProjection(float width, float height) {
    // Update projection matrix to use screen coordinates
    projection = glm::ortho(0.0f, width, 0.0f, height);
    std::cout << "Updated text renderer projection matrix for dimensions: " << width << "x" << height << std::endl;
}

void TextRenderer::renderTestText() {
    if (!shaderProgram) return;
    
    // Simple test rendering
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    
    // Test quad vertices
    GLfloat vertices[6][4] = {
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 0.0f }
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    glUseProgram(0);
}

DebugMenu::DebugMenu()
    : m_window(nullptr)
    , m_game(nullptr)
    , m_isActive(false)
    , m_isTyping(false)
    , m_commandInput("")
    , m_currentInput("")
    , m_historyIndex(-1)
    , m_lastUpdateTime(0.0f)
    , m_cursorVisible(true)
    , m_textRenderer(nullptr)
{
    // TextRenderer will be initialized in the initialize method when we have a valid OpenGL context
}

DebugMenu::~DebugMenu() {
    // Text renderer will be automatically deleted by unique_ptr
}

void DebugMenu::initialize(GLFWwindow* window, Game* game) {
    m_window = window;
    m_game = game;
    
    // Initialize the text renderer now that we have a valid OpenGL context
    std::cout << "Creating TextRenderer in DebugMenu::initialize" << std::endl;
    m_textRenderer = std::make_unique<Render::TextRenderer>();
    
    if (!m_textRenderer->isInitialized()) {
        std::cerr << "ERROR: TextRenderer failed to initialize in DebugMenu::initialize" << std::endl;
    } else {
        std::cout << "TextRenderer successfully initialized in DebugMenu" << std::endl;
    }
    
    // Register built-in commands
    registerCommand("help", "Display available commands", 
        [this](const std::vector<std::string>& args) {
            commandOutput("Available commands:");
            for (const auto& cmd : m_commands) {
                commandOutput("  " + cmd.first + " - " + cmd.second.description);
            }
        });
    
    registerCommand("clear", "Clear the console output",
        [this](const std::vector<std::string>& args) {
            m_commandHistory.clear();
            m_historyIndex = -1;
        });
        
    registerCommand("echo", "Print text to console",
        [this](const std::vector<std::string>& args) {
            std::string message;
            for (size_t i = 1; i < args.size(); i++) {
                message += args[i] + " ";
            }
            commandOutput(message);
        });
    
    // Add an initial welcome message to the command history
    commandOutput("Debug console initialized");
    commandOutput("Press F8 to toggle visibility");
    commandOutput("Type 'help' for a list of commands");
}

bool DebugMenu::handleKeyPress(int key, int action) {
    // Handle F8 key to toggle debug menu
    if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
        std::cout << "F8 key pressed - toggling debug menu" << std::endl;
        toggleVisibility();
        return true;
    }
    
    // Only process other keys if the menu is active
    if (!m_isActive) {
        return false;
    }
    
    // Handle Enter key to execute commands
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        if (!m_commandInput.empty()) {
            // Add to history
            CommandHistoryEntry entry;
            entry.text = m_commandInput;
            entry.isCommand = true;
            m_commandHistory.push_back(entry);
            
            if (m_commandHistory.size() > 20) {
                m_commandHistory.erase(m_commandHistory.begin());
            }
            
            // Process command
            parseCommand(m_commandInput);
            
            // Reset input and history navigation
            m_commandInput.clear();
            m_historyIndex = -1;
        }
        return true;
    }
    
    // Handle backspace for text input
    if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (!m_commandInput.empty()) {
            m_commandInput.pop_back();
        }
        return true;
    }
    
    // Handle escape to close the debug menu
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        toggleVisibility();
        return true;
    }
    
    // Command history navigation with up/down arrows
    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        navigateHistory(true);
        return true;
    }
    
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        navigateHistory(false);
        return true;
    }
    
    return false;
}

void DebugMenu::characterCallback(unsigned int codepoint) {
    if (!m_isActive) {
        return;
    }
    
    // Only allow printable ASCII characters
    if (codepoint >= 32 && codepoint <= 126) {
        m_commandInput += static_cast<char>(codepoint);
    }
}

void DebugMenu::update(float deltaTime) {
    if (!m_isActive) {
        return;
    }
    
    // Update cursor blink state every 0.5 seconds
    m_lastUpdateTime += deltaTime;
    if (m_lastUpdateTime >= 0.5f) {
        m_cursorVisible = !m_cursorVisible;
        m_lastUpdateTime = 0.0f;
    }
}

void DebugMenu::render() {
    if (!m_isActive) {
        return;
    }

    if (!m_window) {
        std::cerr << "Error: Window is null in DebugMenu::render()" << std::endl;
        return;
    }
    
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    std::cout << "===== DEBUG MENU RENDER START =====" << std::endl;
    std::cout << "Window dimensions: " << width << "x" << height << std::endl;
    
    // Save current OpenGL state
    GLfloat prevColor[4];
    glGetFloatv(GL_CURRENT_COLOR, prevColor);
    
    // Use legacy OpenGL for simple drawings
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Change to use same coordinates as UI and Renderer 
    // (0,0 at top-left instead of bottom-left)
    glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable features we don't need for 2D rendering
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    
    if (depthTestEnabled) glDisable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glDisable(GL_CULL_FACE);
    if (!blendEnabled) glEnable(GL_BLEND);
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Keep track of previous OpenGL state for debugging
    std::cout << "Previous OpenGL state:" << std::endl;
    std::cout << "  VAO: " << 0 << std::endl;
    std::cout << "  Program: " << 0 << std::endl;
    std::cout << "  Depth test: " << (depthTestEnabled ? "enabled" : "disabled") << std::endl;
    std::cout << "  Cull face: " << (cullFaceEnabled ? "enabled" : "disabled") << std::endl;
    std::cout << "  Blend: " << (blendEnabled ? "enabled" : "disabled") << std::endl;
    
    // Draw transparent background
    std::cout << "Rendering background box..." << std::endl;
    renderBox(0, 0, width, height);
    
    // Draw a highlighted title bar - make it shorter
    renderBox(0, height - 40.0f, width, 40.0f);
    
    // Render title with debug info - move down to center in the title bar
    float titleY = height - 25.0f;  // Adjusted for smaller title bar
    float titleScale = 0.8f;  // Reduced from 2.0f for better fit
    std::cout << "Rendering title at y=" << titleY << std::endl;
    renderTextAtCenter("Debug Menu (F8)", width / 2.0f, titleY, titleScale);
    
    // Draw command history
    float textScale = 0.5f;  // Reduced from 1.0f for better fit
    float lineHeight = 24.0f;  // Reduced from 48.0f for better spacing
    float startY = height - 120.0f;  // Adjusted from 150.0f for better positioning
    float padding = 20.0f;  // Reduced from 40.0f for better margins
    
    std::cout << "Rendering " << m_commandHistory.size() << " command history entries" << std::endl;
    
    // Draw command history
    for (size_t i = 0; i < m_commandHistory.size(); i++) {
        float y = startY - (i * lineHeight);  // Move up for each line
        if (y < padding) break;
        
        const auto& entry = m_commandHistory[i];
            glm::vec3 color = entry.isCommand ? 
            glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 0.0f);  // Yellow for output, green for commands
                
        std::cout << "Rendering history entry " << i << ": '" << entry.text << "' at y=" << y << std::endl;
        renderText(entry.text, padding, y, textScale, color);
    }
    
    // Draw current input line with cursor
        std::string inputText = "> " + m_commandInput;
        if (m_cursorVisible) {
            inputText += "_";
        }
    std::cout << "Rendering input line: '" << inputText << "' at y=" << padding << std::endl;
    renderText(inputText, padding, padding, textScale, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Restore OpenGL state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    
    if (blendEnabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    
    glColor4fv(prevColor);
}

void DebugMenu::renderBox(float x, float y, float width, float height) {
    // Get window dimensions for proper scaling
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(m_window, &windowWidth, &windowHeight);
    
    // Save current OpenGL state
    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = GL_SRC_ALPHA;
    GLint prevBlendDst = GL_ONE_MINUS_SRC_ALPHA;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);
    
    GLint prevVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);

    GLint prevActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
    
    GLint prevTextureBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTextureBinding);
    
    GLfloat prevColor[4];
    glGetFloatv(GL_CURRENT_COLOR, prevColor);

    // Use immediate mode rendering for simplicity and compatibility
    glUseProgram(0);
        glBindVertexArray(0);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw semi-transparent black background
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);  // Increased alpha from 0.5f to 0.8f
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    
    // Draw border
    glBegin(GL_LINE_LOOP);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f); // White border
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    
    // Restore state
    glBindVertexArray(prevVAO);
    glActiveTexture(prevActiveTexture);
    glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
    
    if (!prevBlend) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(prevBlendSrc, prevBlendDst);
    glUseProgram(prevProgram);
    
    // Restore color
    glColor4fv(prevColor);
}

void DebugMenu::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    // Use TextRenderer if available, otherwise use fallback method
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        m_textRenderer->renderText(text, x, y, scale, color);
    } else {
        // Fallback rendering method
        glColor3f(color.x, color.y, color.z);
        
        float charWidth = 10.0f * scale;
        float charHeight = 20.0f * scale;
        float xpos = x;
        
        for (size_t i = 0; i < text.length(); ++i) {
            renderCharacter(text[i], xpos, y, charWidth, charHeight, scale);
            xpos += charWidth;
        }
    }
}

void DebugMenu::renderTextAtCenter(const std::string& text, float x, float y, float scale) {
    // Calculate text width
    float width = 0.0f;
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        width = m_textRenderer->getTextWidth(text, scale);
    } else {
        width = text.length() * 10.0f * scale;
    }
    
    // Center the text horizontally
    float centerX = x - (width / 2.0f);
    
    // Render with white color
    renderText(text, centerX, y, scale, glm::vec3(1.0f, 1.0f, 1.0f));
    
    Core::StackTrace::log("Rendered centered text: " + text + " at (" + 
                          std::to_string(centerX) + "," + std::to_string(y) + ")");
}

void DebugMenu::renderCharacter(char c, float x, float y, float width, float height, float scale) {
    // Simple line-based character rendering as a fallback
    glLineWidth(scale * 1.5f);
    
    glBegin(GL_LINES);
    switch (c) {
        case 'A':
        case 'a':
            glVertex2f(x + width/2, y + height);
            glVertex2f(x, y);
            glVertex2f(x + width/2, y + height);
            glVertex2f(x + width, y);
            glVertex2f(x + width/4, y + height/2);
            glVertex2f(x + width*3/4, y + height/2);
            break;
        case 'B':
        case 'b':
            glVertex2f(x, y);
            glVertex2f(x, y + height);
            glVertex2f(x, y);
            glVertex2f(x + width*3/4, y);
            glVertex2f(x + width*3/4, y);
            glVertex2f(x + width, y + height/4);
            glVertex2f(x + width, y + height/4);
            glVertex2f(x + width*3/4, y + height/2);
            glVertex2f(x + width*3/4, y + height/2);
            glVertex2f(x, y + height/2);
            glVertex2f(x + width*3/4, y + height/2);
            glVertex2f(x + width, y + height*3/4);
            glVertex2f(x + width, y + height*3/4);
            glVertex2f(x + width*3/4, y + height);
            glVertex2f(x + width*3/4, y + height);
            glVertex2f(x, y + height);
            break;
        // Add more characters as needed
        default:
            // Simple box for any character not specifically implemented
            glVertex2f(x, y);
            glVertex2f(x + width, y);
            glVertex2f(x + width, y);
            glVertex2f(x + width, y + height);
            glVertex2f(x + width, y + height);
            glVertex2f(x, y + height);
            glVertex2f(x, y + height);
            glVertex2f(x, y);
            break;
    }
    glEnd();
}

void DebugMenu::parseCommand(const std::string& input) {
    // Split input into command and arguments
    auto parts = splitString(input, ' ');
    if (parts.empty()) {
        return;
    }
    
    std::string command = parts[0];
    std::transform(command.begin(), command.end(), command.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    // Remove command from parts to get arguments only
    std::vector<std::string> args;
    if (parts.size() > 1) {
        args.assign(parts.begin() + 1, parts.end());
    }
    
    // Look up command in registered commands
    auto it = m_commands.find(command);
    if (it != m_commands.end()) {
        it->second.callback(args);
    } else {
        CommandHistoryEntry output;
        output.text = "Unknown command: " + command;
        output.isCommand = false;
        m_commandHistory.push_back(output);
    }
}

std::vector<std::string> DebugMenu::splitString(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

void DebugMenu::navigateHistory(bool up) {
    if (m_commandHistory.empty()) {
        return;
    }
    
    if (up) {
        // Navigate up through history (older commands)
        if (m_historyIndex < static_cast<int>(m_commandHistory.size()) - 1) {
            m_historyIndex++;
        }
    } else {
        // Navigate down through history (newer commands)
        if (m_historyIndex > -1) {
            m_historyIndex--;
        }
    }
    
    // Set the input to the selected history item or clear it
    if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size()) {
        m_commandInput = m_commandHistory[m_commandHistory.size() - 1 - m_historyIndex].text;
    } else {
        m_commandInput.clear();
    }
}

void DebugMenu::commandOutput(const std::string& text) {
    CommandHistoryEntry output;
    output.text = text;
    output.isCommand = false;
    m_commandHistory.push_back(output);
    
    // Limit the number of output lines to prevent excessive memory usage
    const size_t MAX_OUTPUT_LINES = 100;
    if (m_commandHistory.size() > MAX_OUTPUT_LINES) {
        m_commandHistory.erase(m_commandHistory.begin());
    }
}

void DebugMenu::toggleVisibility() {
    m_isActive = !m_isActive;
    
    std::cout << "===== DEBUG MENU TOGGLE =====" << std::endl;
    std::cout << "Previous state: " << (!m_isActive ? "ACTIVE" : "INACTIVE") << std::endl;
    std::cout << "New state: " << (m_isActive ? "ACTIVE" : "INACTIVE") << std::endl;
    std::cout << "Window handle: " << (m_window ? "VALID" : "NULL") << std::endl;
    
    if (!m_window) {
        std::cerr << "ERROR: Window handle is null when toggling debug menu!" << std::endl;
        return;
    }
    
    // Add a message to the output area when toggling
    if (m_isActive) {
        CommandHistoryEntry entry;
        entry.text = "Debug menu activated";
        entry.isCommand = false;
        m_commandHistory.push_back(entry);
        
        // Get current window size for debug info
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        
        std::cout << "Window dimensions: " << width << "x" << height << std::endl;
        
        // Immediately set cursor to normal mode
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        // Center cursor in the window to avoid accidental clicks
        glfwSetCursorPos(m_window, width/2, height/2);
        
        // Add debug info to output
        std::stringstream ss;
        ss << "Window size: " << width << "x" << height;
        entry.text = ss.str();
        m_commandHistory.push_back(entry);
    } else {
        CommandHistoryEntry entry;
        entry.text = "Debug menu deactivated";
        entry.isCommand = false;
        m_commandHistory.push_back(entry);
        
        // Setting cursor to disabled mode
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    std::cout << "===== END DEBUG MENU TOGGLE =====" << std::endl;
}

void DebugMenu::registerCommand(const std::string& name, 
                             const std::string& description,
                             std::function<void(const std::vector<std::string>&)> callback) {
    Command cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.callback = callback;
    
    m_commands[name] = cmd;
}

void DebugMenu::renderSplashScreen() {
    if (!m_textRenderer || !m_textRenderer->isInitialized()) {
        std::cerr << "ERROR: Cannot render splash screen - text renderer not initialized" << std::endl;
        return;
    }

    if (!m_window) {
        std::cerr << "ERROR: Cannot render splash screen - window is null" << std::endl;
        return;
    }

    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    // Update projection matrix
    m_textRenderer->updateProjection(width, height);

    // Render splash screen text
    std::string splashText = "Welcome to VoxelGame!";
    float textScale = 2.0f;
    float textX = (width - m_textRenderer->getTextWidth(splashText, textScale)) / 2.0f;
    float textY = height / 2.0f;

    m_textRenderer->renderText(splashText, textX, textY, textScale, glm::vec3(1.0f, 1.0f, 1.0f));
}

} // namespace Debug 