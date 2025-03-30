#include "render/TextRenderer.hpp"
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <algorithm>

namespace Render {

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
        // Project resources directory (highest priority)
        "resources/fonts/DejaVuSans.ttf",
        "resources/fonts/DejaVuSans-Bold.ttf",
        "resources/fonts/NimbusSans-Regular.otf",
        "resources/fonts/NimbusSans-Bold.otf",
        "resources/fonts/Arial.ttf",
        "resources/fonts/arial.ttf",
        
        // Linux paths (prioritized)
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/opentype/urw-base35/NimbusSans-Regular.otf",
        "/usr/share/fonts/opentype/urw-base35/NimbusSans-Bold.otf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        
        // Windows paths
        "C:\\Windows\\Fonts\\Arial.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        
        // macOS paths
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    };

    bool fontLoaded = false;
    for (const char* fontPath : fontPaths) {
        std::cout << "Attempting to load font: " << fontPath << std::endl;
        
        // Check if file exists
        std::filesystem::path path(fontPath);
        if (std::filesystem::exists(path)) {
            std::cout << "Font file exists: " << fontPath << std::endl;
        } else {
            std::cout << "Font file does not exist: " << fontPath << std::endl;
            continue;
        }
        
        if (FT_New_Face(ft, fontPath, 0, &face) == 0) {
            std::cout << "Successfully loaded font: " << fontPath << std::endl;
            fontLoaded = true;
            break;
        } else {
            std::cerr << "Failed to load font: " << fontPath << std::endl;
        }
    }

    if (!fontLoaded) {
        std::cerr << "ERROR::FREETYPE: Failed to load any system font. Text rendering will be disabled." << std::endl;
        FT_Done_FreeType(ft);
        ft = nullptr;
        return;
    }

    // Set size to load glyphs as - larger for better visibility
    FT_Set_Pixel_Sizes(face, 0, 48);  // Increased from 32 to 48 for better legibility
    std::cout << "Set font pixel size to 48" << std::endl;

    // Create and compile the shader program
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader) {
        std::cerr << "ERROR::SHADER: Failed to create vertex shader" << std::endl;
        return;
    }
    
    const char* vertexShaderSource = 
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex; // vec2 pos, vec2 tex\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
        "   TexCoords = vertex.zw;\n"
        "}\n";
    
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for shader compile errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader) {
        std::cerr << "ERROR::SHADER: Failed to create fragment shader" << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    
    const char* fragmentShaderSource = 
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n"
        "   vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "   color = vec4(textColor, 1.0) * sampled;\n"
        "}\n";
    
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    
    // Link shaders
    shaderProgram = glCreateProgram();
    if (!shaderProgram) {
        std::cerr << "ERROR::SHADER: Failed to create shader program" << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return;
    }
    
    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Get uniform locations
    textUniformLoc = glGetUniformLocation(shaderProgram, "text");
    textColorUniformLoc = glGetUniformLocation(shaderProgram, "textColor");
    projectionUniformLoc = glGetUniformLocation(shaderProgram, "projection");
    
    if (textUniformLoc == -1 || textColorUniformLoc == -1 || projectionUniformLoc == -1) {
        std::cerr << "ERROR::SHADER: Failed to get uniform locations: "
                  << "text=" << textUniformLoc 
                  << ", textColor=" << textColorUniformLoc 
                  << ", projection=" << projectionUniformLoc << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return;
    }

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++) {
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
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Now store character for later use
        Character character = {
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    if (VAO == 0 || VBO == 0) {
        std::cerr << "ERROR: Failed to generate VAO/VBO. VAO: " << VAO << ", VBO: " << VBO << std::endl;
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        VAO = VBO = 0;
        return;
    } else {
        std::cout << "Successfully generated VAO: " << VAO << ", VBO: " << VBO << std::endl;
    }
    
    // Exactly match the text_test binding and configuration sequence
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Set up default projection matrix (will be updated later with actual screen dimensions)
    projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
    
    std::cout << "TextRenderer loaded " << Characters.size() << " characters" << std::endl;

    // Check if initialization was successful
    if (!shaderProgram || !VAO || !VBO || Characters.empty()) {
        std::cerr << "ERROR: TextRenderer initialization failed!" << std::endl;
        return;
    }

    std::cout << "TextRenderer initialized successfully" << std::endl;
}

TextRenderer::~TextRenderer() {
    // Cleanup FreeType resources
    if (face) {
        FT_Done_Face(face);
        face = nullptr;
    }
    
    if (ft) {
        FT_Done_FreeType(ft);
        ft = nullptr;
    }

    // Cleanup OpenGL resources
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    
    // Delete all character textures
    for (auto& ch : Characters) {
        glDeleteTextures(1, &ch.second.TextureID);
    }
    Characters.clear();
    
    std::cout << "TextRenderer destroyed" << std::endl;
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    if (!isInitialized()) {
        std::cerr << "TextRenderer not initialized, cannot render text." << std::endl;
        renderFallbackText(text, x, y, scale, color);
        return;
    }

    // Debug output to help diagnose the issue
    std::cout << "Rendering text: '" << text << "' at (" << x << "," << y << ") with scale " << scale << std::endl;
    std::cout << "Using VAO: " << VAO << ", VBO: " << VBO << ", shader: " << shaderProgram << std::endl;
    std::cout << "Character map size: " << Characters.size() << std::endl;

    // Check all GL errors before we start
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error before rendering: " << err << std::endl;
        // Clear all errors
        while (err != GL_NO_ERROR) {
            err = glGetError();
        }
    }

    // Set up OpenGL state exactly like text_test.cpp
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(shaderProgram);
    
    // Check for any errors after program binding
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after shader program binding: " << err << std::endl;
        return;
    }

    glUniformMatrix4fv(projectionUniformLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Check for errors after setting projection
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after projection matrix uniform: " << err << std::endl;
        return;
    }
    
    glUniform3f(textColorUniformLoc, color.x, color.y, color.z);
    
    // Check for errors after setting text color
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after text color uniform: " << err << std::endl;
        return;
    }
    
    glUniform1i(textUniformLoc, 0); // Set texture unit to 0
    
    // Check for errors after setting texture sampler
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after texture sampler uniform: " << err << std::endl;
        return;
    }
    
    glActiveTexture(GL_TEXTURE0);
    
    // Check for errors after activating texture unit
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after activating texture unit: " << err << std::endl;
        return;
    }
    
    glBindVertexArray(VAO);
    
    // Check for errors after binding VAO
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error after binding VAO: " << err << std::endl;
        return;
    }

    // Iterate through all characters
    for (auto c : text) {
        auto it = Characters.find(c);
        if (it == Characters.end()) {
            std::cerr << "Warning: Character '" << c << "' not found in texture atlas" << std::endl;
            continue;
        }
        
        Character ch = it->second;
        
        // FIX: Modified for top-left origin coordinates
        // For top-left origin (0,0 at top-left), we need to adjust the Y calculation
        // In top-left origin, increasing Y moves DOWN the screen
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y + ch.Bearing.y * scale - ch.Size.y * scale;  // Adjusted for top-left origin

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        
        std::cout << "DEBUG Character '" << c << "': position=(" << xpos << "," << ypos << "), size=(" 
                  << w << "," << h << "), bearing=(" << ch.Bearing.x << "," << ch.Bearing.y << ")" << std::endl;

        // Define the quad vertices with texture coordinates for top-left origin system
        // In top-left system: (0,0) is top-left, (1,1) is bottom-right for texture coordinates
        float vertices[6][4] = {
            { xpos,     ypos,       0.0f, 0.0f },            // Top-left vertex, top-left texture coord
            { xpos,     ypos + h,   0.0f, 1.0f },            // Bottom-left vertex, bottom-left texture
            { xpos + w, ypos + h,   1.0f, 1.0f },            // Bottom-right vertex, bottom-right texture

            { xpos,     ypos,       0.0f, 0.0f },            // Top-left vertex, top-left texture coord
            { xpos + w, ypos + h,   1.0f, 1.0f },            // Bottom-right vertex, bottom-right texture
            { xpos + w, ypos,       1.0f, 0.0f }             // Top-right vertex, top-right texture
        };

        // Bind the glyph texture
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
        // Check for errors after binding texture
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after binding texture: " << err << " for character '" << c << "' with TextureID: " << ch.TextureID << std::endl;
            continue;
        }
        
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        // Check for errors after binding VBO
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after binding VBO: " << err << std::endl;
            continue;
        }
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        // Check for errors after buffer data update
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after buffer data update: " << err << std::endl;
            continue;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Check for errors after drawing
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after drawing character: " << err << std::endl;
            continue;
        }
        
        // Now advance cursors for next glyph
        x += (ch.Advance >> 6) * scale;
    }
    
    // Reset OpenGL state
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

float TextRenderer::getTextWidth(const std::string& text, float scale) const {
    float width = 0.0f;
    for (auto c : text) {
        auto it = Characters.find(c);
        if (it != Characters.end()) {
            width += (it->second.Advance >> 6) * scale;
        }
    }
    return width;
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
    
    // Set up for simple rendering
    glColor3f(color.x, color.y, color.z);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float charWidth = 10.0f * scale;
    float charHeight = 20.0f * scale;
    
    // Now draw each character
    for (unsigned int i = 0; i < text.length(); i++) {
        char c = text[i];
        float xpos = x + i * charWidth;
        
        // Draw the actual character - just simple lines
        glLineWidth(1.5f * scale);
        glColor3f(color.x, color.y, color.z);
        glBegin(GL_LINES);
        
        // Simplified character drawing
        switch (c) {
            case 'A':
                glVertex2f(xpos + charWidth * 0.2f, y);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight);
                glVertex2f(xpos + charWidth * 0.5f, y + charHeight);
                glVertex2f(xpos + charWidth * 0.8f, y);
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.6f);
                break;
            case 'B':
                glVertex2f(xpos + charWidth * 0.2f, y);
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight);
                glVertex2f(xpos + charWidth * 0.2f, y);
                glVertex2f(xpos + charWidth * 0.7f, y);
                glVertex2f(xpos + charWidth * 0.7f, y);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.2f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.4f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.8f, y + charHeight * 0.6f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight);
                glVertex2f(xpos + charWidth * 0.2f, y + charHeight);
                break;
            // ... more character cases can be added here
            default:
                // Just draw a simple line or box for unimplemented characters
                glVertex2f(xpos + charWidth * 0.3f, y + charHeight * 0.5f);
                glVertex2f(xpos + charWidth * 0.7f, y + charHeight * 0.5f);
                break;
        }
        glEnd();
    }
    
    // Restore previous state
    glColor4fv(prevColor);
    if (!prevBlend) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(prevBlendSrcAlpha, prevBlendDstAlpha);
}

void TextRenderer::updateProjection(float width, float height) {
    // Update projection matrix to use screen coordinates with (0,0) at top-left corner
    // to match GLFW window coordinates and the rest of the UI system
    projection = glm::ortho(0.0f, width, height, 0.0f);
    std::cout << "Updated text renderer projection matrix for dimensions: " << width << "x" << height 
              << " with top-left origin" << std::endl;
}

void TextRenderer::renderTestText() {
    if (!isInitialized()) {
        std::cerr << "TextRenderer not initialized - cannot render test text" << std::endl;
        return;
    }
    
    // Render some test text with a smaller scale
    try {
        renderText("Test", 25.0f, 25.0f, 0.5f, glm::vec3(0.5f, 0.8f, 0.2f));
        renderText("Text Renderer", 25.0f, 75.0f, 0.5f, glm::vec3(0.3f, 0.7f, 0.9f));
    } catch (const std::exception& e) {
        std::cerr << "Error during test text rendering: " << e.what() << std::endl;
    }
}

} // namespace Render 