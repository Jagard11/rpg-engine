#!/bin/bash
# Modified build.sh - Removed software rendering enforcement
# Original script forced software rendering which prevented hardware OpenGL acceleration

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

# Run CMake
echo "Running CMake..."
cmake .. || { echo "CMake failed"; exit 1; }

# Build the project
echo "Building..."
cmake --build . || { echo "Build failed"; exit 1; }


# Clear any environment variables that force software rendering
unset LIBGL_ALWAYS_SOFTWARE
unset QT_OPENGL

# Set environment variables to explicitly enable hardware acceleration
export QTWEBENGINE_CHROMIUM_FLAGS="--enable-gpu --enable-webgl"
export QT_OPENGL=desktop

# If we get here, the build was successful
echo "Build successful! Running application..."
./OobaboogaRPGArena