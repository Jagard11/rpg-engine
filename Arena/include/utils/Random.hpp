#pragma once

#include <random>
#include <cstdint>

class Random {
public:
    Random(uint64_t seed = std::random_device{}());
    
    // Basic random number generation
    float getFloat(float min = 0.0f, float max = 1.0f);
    int getInt(int min, int max);
    
    // Noise generation
    float noise1D(float x) const;
    float noise2D(float x, float y) const;
    float noise3D(float x, float y, float z) const;
    
    // Fractal noise (multiple octaves)
    float fractalNoise2D(float x, float y, int octaves, float persistence) const;
    float fractalNoise3D(float x, float y, float z, int octaves, float persistence) const;
    
    // Utility functions
    void setSeed(uint64_t seed);
    uint64_t getSeed() const { return m_seed; }

private:
    // Perlin noise helpers
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y, float z) const;
    int hash(int x) const;
    
    std::mt19937_64 m_generator;
    uint64_t m_seed;
    
    // Permutation table for Perlin noise
    static constexpr int PERM_SIZE = 256;
    int m_perm[PERM_SIZE];
}; 