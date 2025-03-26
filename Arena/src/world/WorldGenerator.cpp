#include "world/WorldGenerator.hpp"
#include <glm/gtc/noise.hpp>

WorldGenerator::WorldGenerator(uint64_t seed)
    : m_random(seed)
    , m_seed(seed)
{
}

WorldGenerator::~WorldGenerator() {
}

float WorldGenerator::getHeight(int x, int z) const {
    float height = getNoise2D(x / TERRAIN_SCALE, z / TERRAIN_SCALE);
    return BASE_HEIGHT + height * HEIGHT_SCALE;
}

uint8_t WorldGenerator::getBlockType(int x, int y, int z) const {
    float terrainHeight = getHeight(x, z);
    
    if (y > terrainHeight) {
        return 0; // Air
    }
    
    if (y == static_cast<int>(terrainHeight)) {
        return 1; // Grass
    }
    
    if (y > terrainHeight - 4) {
        return 2; // Dirt
    }
    
    return 3; // Stone
}

float WorldGenerator::getNoise(float x, float y, float z) const {
    return glm::perlin(glm::vec3(x, y, z));
}

float WorldGenerator::getNoise2D(float x, float z) const {
    const float PERSISTENCE = 0.5f;
    const int OCTAVES = 4;
    
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < OCTAVES; ++i) {
        total += glm::perlin(glm::vec2(x * frequency, z * frequency)) * amplitude;
        maxValue += amplitude;
        amplitude *= PERSISTENCE;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
} 