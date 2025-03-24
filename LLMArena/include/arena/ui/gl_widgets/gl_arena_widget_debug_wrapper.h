// include/arena/ui/gl_widgets/gl_arena_widget_debug_wrapper.h
#ifndef GL_ARENA_WIDGET_DEBUG_WRAPPER_H
#define GL_ARENA_WIDGET_DEBUG_WRAPPER_H

#include <QObject>
#include <QVariant>

// This wrapper header ensures that quintptr is only declared as a metatype once
// and serves as a common include point for any file that needs to use quintptr
// with QVariant in the debug system.

// Only declare the metatype if not already declared
#ifndef QUINTPTR_METATYPE_DECLARED
#define QUINTPTR_METATYPE_DECLARED
Q_DECLARE_METATYPE(quintptr)
#endif

#endif // GL_ARENA_WIDGET_DEBUG_WRAPPER_H