// src/arena/ui/gl_widgets/gl_arena_widget_debug_wrapper.h
#ifndef GL_ARENA_WIDGET_DEBUG_WRAPPER_H
#define GL_ARENA_WIDGET_DEBUG_WRAPPER_H

// This is a wrapper header that includes the necessary headers for debug functionality
// It resolves issues with incomplete types by including full definitions

#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../include/arena/debug/debug_system.h" 
#include "../../../include/arena/debug/console/debug_console.h"
#include "../../../include/arena/debug/visualizers/frustum_visualizer.h"

// Register QPaintDevice pointer as a metatype for QVariant usage
Q_DECLARE_METATYPE(quintptr)

// This ensures all the necessary types are fully defined before they are used
// in the implementation files that include this wrapper

#endif // GL_ARENA_WIDGET_DEBUG_WRAPPER_H