// include/arena/ui/gl_widgets/gl_arena_widget_debug.h
#ifndef GL_ARENA_WIDGET_DEBUG_H
#define GL_ARENA_WIDGET_DEBUG_H

#include <QObject>

// This header contains debug-related method declarations for GLArenaWidget
// To be included by gl_arena_widget.h

// Debug-related methods to be added to GLArenaWidget
private:
    // Initialize the debug system
    void initializeDebugSystem();
    
    // Render debug overlays
    void renderDebugOverlays();
    
    // Handle debug key presses
    bool handleDebugKeyPress(QKeyEvent* event);
    
    // Toggle debug visualizer
    void toggleDebugVisualizer(int visualizerType);
    
    // Check if debug console is visible
    bool isDebugConsoleVisible() const;
    
    // Check if debug visualizer is enabled
    bool isDebugVisualizerEnabled(int visualizerType) const;

#endif // GL_ARENA_WIDGET_DEBUG_H