#include "utils/Random.hpp"

Random::Random(uint64_t seed)
    : m_seed(seed)
{
    setSeed(seed);
}

float Random::getFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_generator);
}

int Random::getInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_generator);
}

void Random::setSeed(uint64_t seed) {
    m_seed = seed;
    m_generator.seed(seed);
} 