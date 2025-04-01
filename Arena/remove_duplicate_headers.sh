#!/bin/bash

echo "Creating backup of duplicate header files..."
mkdir -p header_backup/world
mkdir -p header_backup/renderer

echo "Backing up src/world/World.hpp"
cp src/world/World.hpp header_backup/world/ 2>/dev/null || echo "File not found"

echo "Backing up src/world/Chunk.hpp"
cp src/world/Chunk.hpp header_backup/world/ 2>/dev/null || echo "File not found"

echo "Backing up src/world/ChunkVisibilityManager.hpp"
cp src/world/ChunkVisibilityManager.hpp header_backup/world/ 2>/dev/null || echo "File not found"

echo "Backing up src/renderer/Renderer.hpp"
cp src/renderer/Renderer.hpp header_backup/renderer/ 2>/dev/null || echo "File not found"

echo ""
echo "Removing duplicate header files..."

echo "Removing src/world/World.hpp"
git rm src/world/World.hpp 2>/dev/null || rm -f src/world/World.hpp 2>/dev/null || echo "File not found or already removed"

echo "Removing src/world/Chunk.hpp"
git rm src/world/Chunk.hpp 2>/dev/null || rm -f src/world/Chunk.hpp 2>/dev/null || echo "File not found or already removed"

echo "Removing src/world/ChunkVisibilityManager.hpp"
git rm src/world/ChunkVisibilityManager.hpp 2>/dev/null || rm -f src/world/ChunkVisibilityManager.hpp 2>/dev/null || echo "File not found or already removed"

echo "Removing src/renderer/Renderer.hpp"
git rm src/renderer/Renderer.hpp 2>/dev/null || rm -f src/renderer/Renderer.hpp 2>/dev/null || echo "File not found or already removed"

echo ""
echo "Cleanup complete. Backup files are stored in the header_backup directory."
echo "Now try rebuilding your project to ensure everything still compiles correctly."
echo "If you have any issues, you can restore the backup files from the header_backup directory."
echo ""
echo "Next steps:"
echo "1. Commit these changes to version control"
echo "2. Update your documentation to clearly state that all header files should be in the include/ directory"
echo "3. Consider removing the src/ directory from your include paths in CMakeLists.txt to enforce this guideline" 