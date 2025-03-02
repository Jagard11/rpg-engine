#!/bin/bash

# Navigate to the project directory
cd /media/jagard/Disk\ 0/GIT/rpg-engine/GameFPS/VoxelGlobe || { echo "Failed to navigate to directory"; exit 1; }

# Remove CMake cache and files for a clean build
rm -rf CMakeCache.txt CMakeFiles/

# Run cmake and make, exit on failure
cmake . || { echo "CMake configuration failed"; exit 1; }
make || { echo "Make failed"; exit 1; }

# Run the executable, exit on failure
./VoxelGlobe || { echo "Execution failed"; exit 1; }