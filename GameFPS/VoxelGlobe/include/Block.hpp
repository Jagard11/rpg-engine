#ifndef BLOCK_HPP
#define BLOCK_HPP

enum class BlockType { AIR, DIRT, GRASS }; // Simple for now

struct Block {
    BlockType type;
    Block(BlockType t = BlockType::AIR) : type(t) {}
};

#endif