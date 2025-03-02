// ./include/World/Block.hpp
#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "Core/Types.hpp"

struct Block {
    BlockType type;
    Block(BlockType t = BlockType::AIR) : type(t) {}
};

#endif