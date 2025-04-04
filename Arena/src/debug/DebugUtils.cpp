#include "debug/DebugUtils.hpp"
#include "core/StackTrace.hpp"
#include <GL/glew.h>
#include <iostream>

namespace Debug {

void checkGLError(const char* location) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << location << ": ";
        switch (error) {
            case GL_INVALID_ENUM: std::cerr << "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: std::cerr << "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: std::cerr << "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW: std::cerr << "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: std::cerr << "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY: std::cerr << "GL_OUT_OF_MEMORY"; break;
            default: std::cerr << "Unknown error " << error; break;
        }
        std::cerr << std::endl;
        Core::StackTrace::printStackTrace();
    }
}

} // namespace Debug 