#!/bin/bash
# debug-build.sh - Simplified script with debugging

# Set environment variables for WebGL rendering in software mode
export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu --disable-gpu-compositing --enable-software-compositing"
export LIBGL_ALWAYS_SOFTWARE=1
export QT_OPENGL=software

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

# If we get here, the build was successful
echo "Build successful! Running application..."
./OobaboogaRPGArena