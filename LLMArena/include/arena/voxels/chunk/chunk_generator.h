// include/arena/voxels/chunk/chunk_generator.h
#ifndef CHUNK_GENERATOR_H
#define CHUNK_GENERATOR_H

#include <memory>
#include <random>
#include "chunk.h"

/**
 * @brief Abstract base class for chunk generators.
 * 
 * Chunk generators are responsible for creating the initial content of chunks.
 * This can be based on noise functions, patterns, or any other algorithm.
 */
class ChunkGenerator {
public:
    virtual ~ChunkGenerator() = default;
    
    /**
     * @brief Generate a chunk at the given coordinate
     * @param coordinate Chunk coordinate
     * @return The generated chunk
     */
    virtual std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate) = 0;
    
    /**
     * @brief Set the seed for the generator
     * @param seed Random seed
     */
    virtual void setSeed(unsigned int seed) = 0;
};

/**
 * @brief Basic flat terrain generator.
 * 
 * Creates a flat world with a ground layer at y=0.
 */
class FlatTerrainGenerator : public ChunkGenerator {
public:
    FlatTerrainGenerator();
    
    std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate) override;
    void setSeed(unsigned int seed) override;
    
private:
    unsigned int m_seed;
    std::mt19937 m_random;
};

/**
 * @brief Noise-based terrain generator.
 * 
 * Creates rolling hills and valleys using simplex noise.
 */
class NoiseTerrainGenerator : public ChunkGenerator {
public:
    NoiseTerrainGenerator();
    
    std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate) override;
    void setSeed(unsigned int seed) override;
    
    // Configure noise parameters
    void setFrequency(float frequency) { m_frequency = frequency; }
    void setAmplitude(float amplitude) { m_amplitude = amplitude; }
    void setOctaves(int octaves) { m_octaves = octaves; }
    void setLacunarity(float lacunarity) { m_lacunarity = lacunarity; }
    void setPersistence(float persistence) { m_persistence = persistence; }
    
    // Noise generation methods - making them public so they can be used by other generators
    float getNoise(float x, float y) const;
    float getFractalNoise(float x, float y) const;
    
private:
    unsigned int m_seed;
    std::mt19937 m_random;
    
    // Noise parameters
    float m_frequency;
    float m_amplitude;
    int m_octaves;
    float m_lacunarity;
    float m_persistence;
};

/**
 * @brief Spherical planet generator.
 * 
 * Creates a spherical planet with terrain based on noise.
 * This is used for globe-scale world generation.
 */
class SphericalPlanetGenerator : public ChunkGenerator {
public:
    SphericalPlanetGenerator();
    
    std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate) override;
    void setSeed(unsigned int seed) override;
    
    // Configure planet parameters
    void setRadius(float radius) { m_radius = radius; }
    void setTerrainHeight(float height) { m_terrainHeight = height; }
    void setSeaLevel(float level) { m_seaLevel = level; }
    
private:
    unsigned int m_seed;
    std::mt19937 m_random;
    
    // Planet parameters
    float m_radius;         // Base radius of the planet
    float m_terrainHeight;  // Maximum height of terrain above base radius
    float m_seaLevel;       // Sea level (0-1, percentage of terrain height)
    
    // Noise generator for terrain
    std::unique_ptr<NoiseTerrainGenerator> m_noiseGenerator;
    
    // Helper methods
    QVector3D sphericalToCartesian(float longitude, float latitude, float radius) const;
    void cartesianToSpherical(const QVector3D& pos, float& longitude, float& latitude, float& radius) const;
    bool isPointInSphere(const QVector3D& point, float radius) const;
};

/**
 * @brief Enhanced noise-based terrain generator with seamless chunk boundaries.
 * 
 * Creates procedural terrain that is continuous across chunk boundaries
 * and provides methods for finding surface height at specific coordinates.
 */
class ImprovedTerrainGenerator : public ChunkGenerator {
public:
    ImprovedTerrainGenerator();
    
    std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate) override;
    void setSeed(unsigned int seed) override;
    
    // Configure noise parameters
    void setFrequency(float frequency) { m_frequency = frequency; }
    void setAmplitude(float amplitude) { m_amplitude = amplitude; }
    void setOctaves(int octaves) { m_octaves = octaves; }
    void setLacunarity(float lacunarity) { m_lacunarity = lacunarity; }
    void setPersistence(float persistence) { m_persistence = persistence; }
    
    // Get the surface height at a specific world position
    float getSurfaceHeightAt(float x, float z) const;
    
    // Determine if a block at specific coordinates should be solid
    bool isSolid(float x, float y, float z) const;
    
    // Noise generation methods
    float getNoise(float x, float z) const;
    float getFractalNoise(float x, float z) const;
    
private:
    unsigned int m_seed;
    std::mt19937 m_random;
    
    // Feature generation
    void generateTree(std::shared_ptr<Chunk> chunk, int x, int y, int z);
    void generateRock(std::shared_ptr<Chunk> chunk, int x, int y, int z);
    
    // Helper methods for chunk boundary seamless detection
    bool isNearChunkBoundary(int localX, int localZ) const;
    
    // Noise parameters
    float m_frequency;
    float m_amplitude;
    int m_octaves;
    float m_lacunarity;
    float m_persistence;
    
    // Biome parameters
    float m_grasslandThreshold;
    float m_mountainThreshold;
    float m_snowThreshold;
};

#endif // CHUNK_GENERATOR_H