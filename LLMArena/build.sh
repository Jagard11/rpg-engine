#!/bin/bash
# Improved build.sh with better OpenGL configuration

# Print current directory
echo "Current directory: $(pwd)"
echo "Script location: $0"
echo "--------------------------------------"

# List files to verify script environment
echo "Files in current directory:"
ls -la
echo "--------------------------------------"

# Check if build directory exists
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build || { echo "Failed to create/enter build directory"; exit 1; }

# Set environment variables for OpenGL
export QT_OPENGL=desktop
export QT_LOGGING_RULES="qt.qpa.gl=true"
unset QT_SCALE_FACTOR  # Let Qt handle scaling automatically
unset QT_AUTO_SCREEN_SCALE_FACTOR
unset QT_SCREEN_SCALE_FACTORS

# Unset any variables that might force software rendering
unset LIBGL_ALWAYS_INDIRECT
unset LIBGL_ALWAYS_SOFTWARE
unset LIBGL_DRI3_DISABLE
unset GALLIUM_DRIVER

# Run CMake with proper options
echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. || { echo "CMake failed"; exit 1; }

# Build the project
echo "Building..."
cmake --build . || { echo "Build failed"; exit 1; }

# Set explicit environment variables for OpenGL hardware acceleration
export QT_OPENGL=desktop
export QT_QUICK_BACKEND=software  # Use software backend for Qt Quick (not used in this app)
export QSG_RENDER_LOOP=basic      # Use basic render loop for stability
export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu"  # Disable GPU for WebEngine, not used in this version
export QT_QPA_PLATFORM=xcb        # Use X11 platform on Linux

echo "Build successful! Running application with hardware OpenGL enabled..."
./OobaboogaRPGArena