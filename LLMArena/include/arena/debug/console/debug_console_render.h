// include/arena/debug/console/debug_console_render.h
#ifndef DEBUG_CONSOLE_RENDER_H
#define DEBUG_CONSOLE_RENDER_H

// This file contains rendering-related method declarations for DebugConsole
// To be included by debug_console.h

private:
    // Draw console text using QPainter
    void drawConsoleText(int screenWidth, int screenHeight, float consoleHeight);

#endif // DEBUG_CONSOLE_RENDER_H