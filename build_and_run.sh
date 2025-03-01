#!/bin/bash

# Navigate to the project directory (adjust the path if needed)
cd /media/jagard/Disk\ 0/GIT/rpg-engine/GameFPS/VoxelGlobe

# Remove CMake cache and files
rm -rf CMakeCache.txt CMakeFiles/

# Run cmake and make
cmake . && make

# Run the executable
./VoxelGlobe