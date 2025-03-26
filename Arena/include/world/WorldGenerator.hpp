#pragma once

#include <glm/glm.hpp>
#include "../utils/Random.hpp"

class WorldGenerator {
public:
    WorldGenerator(uint64_t seed);
    ~WorldGenerator();

    // Generate terrain height at a given position
    float getHeight(int x, int z) const;
    
    // Get block type at a position
    uint8_t getBlockType(int x, int y, int z) const;

private:
    // Noise generation
    float getNoise(float x, float y, float z) const;
    float getNoise2D(float x, float z) const;
    
    // Generation parameters
    static constexpr float TERRAIN_SCALE = 100.0f;
    static constexpr float HEIGHT_SCALE = 32.0f;
    static constexpr int BASE_HEIGHT = 64;
    
    Random m_random;
    uint64_t m_seed;
}; 