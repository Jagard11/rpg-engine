// ./VoxelGlobe/include/Block.hpp
#ifndef BLOCK_HPP
#define BLOCK_HPP

enum class BlockType {
    AIR,
    DIRT,
    GRASS
};

struct Block {
    BlockType type;
    Block(BlockType t = BlockType::AIR) : type(t) {}
};

#endif