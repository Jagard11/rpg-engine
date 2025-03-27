#pragma once

namespace Debug {

/**
 * @brief Checks for OpenGL errors and prints them
 * @param location A string identifying where the check is being made
 */
void checkGLError(const char* location);

} // namespace Debug 