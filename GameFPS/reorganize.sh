#!/bin/bash

BASE_DIR="VoxelGlobe"
SRC_DIR="$BASE_DIR/src"
INCLUDE_DIR="$BASE_DIR/include"

echo "Updating include paths..."

# Function to update includes in a file
update_file() {
    local file=$1
    sed -i 's|#include "Debug.hpp"|#include "Core/Debug.hpp"|' "$file"
    sed -i 's|#include "Block.hpp"|#include "World/Block.hpp"|' "$file"
    sed -i 's|#include "Chunk.hpp"|#include "World/Chunk.hpp"|' "$file"
    sed -i 's|#include "World.hpp"|#include "World/World.hpp"|' "$file"
    sed -i 's|#include "Player.hpp"|#include "Player/Player.hpp"|' "$file"
    sed -i 's|#include "Renderer.hpp"|#include "Rendering/Renderer.hpp"|' "$file"
    sed -i 's|#include "imgui.h"|#include "../../third_party/imgui/imgui.h"|' "$file"
    sed -i 's|#include "stb_image.h"|#include "../../third_party/stb/stb_image.h"|' "$file"
}

# Apply updates to all .cpp and .hpp files
find "$SRC_DIR" "$INCLUDE_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) | while read -r file; do
    update_file "$file"
    echo "Updated: $file"
done

echo "Include path updates complete!"