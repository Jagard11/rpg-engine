// src/arena/voxels/chunk/chunk_generator.cpp
#include "../../../../include/arena/voxels/chunk/chunk_generator.h"
#include <QDebug>
#include <cmath>

//-------------------------------------------------------------------------
// FlatTerrainGenerator implementation
//-------------------------------------------------------------------------

FlatTerrainGenerator::FlatTerrainGenerator()
    : m_seed(0) {
    // Initialize random generator with default seed
    m_random.seed(m_seed);
}

std::shared_ptr<Chunk> FlatTerrainGenerator::generateChunk(const ChunkCoordinate& coordinate) {
    // Create a new chunk
    auto chunk = std::make_shared<Chunk>(coordinate);
    
    // Only generate terrain for chunks at or below Y=0
    if (coordinate.getY() > 0) {
        return chunk; // Empty chunk above ground
    }
    
    // Generate flat terrain
    for (int x = 0; x < ChunkCoordinate::CHUNK_SIZE; x++) {
        for (int z = 0; z < ChunkCoordinate::CHUNK_SIZE; z++) {
            // Get world coordinates for noise
            int worldX = coordinate.getX() * ChunkCoordinate::CHUNK_SIZE + x;
            int worldZ = coordinate.getZ() * ChunkCoordinate::CHUNK_SIZE + z;
            
            // Determine materials based on world position for variety
            VoxelType surfaceType;
            QColor surfaceColor;
            
            // Simple pattern based on world coordinates
            bool isGrass = ((worldX / 16) % 2) ^ ((worldZ / 16) % 2);
            
            if (isGrass) {
                surfaceType = VoxelType::Grass;
                surfaceColor = QColor(34, 139, 34); // Forest green
            } else {
                surfaceType = VoxelType::Dirt;
                surfaceColor = QColor(139, 69, 19); // Saddle brown
            }
            
            // Surface layer (y = 15 in the chunk at y=0)
            if (coordinate.getY() == 0) {
                chunk->setVoxel(x, 15, z, Voxel(surfaceType, surfaceColor));
                
                // Dirt beneath the surface
                for (int y = 0; y < 15; y++) {
                    chunk->setVoxel(x, y, z, Voxel(VoxelType::Dirt, QColor(139, 69, 19)));
                }
            }
            else {
                // Below y=0 chunks are entirely dirt/stone
                int depth = -coordinate.getY();
                
                for (int y = 0; y < ChunkCoordinate::CHUNK_SIZE; y++) {
                    // Deeper layers have stone
                    if (depth > 1 || y < 10) {
                        chunk->setVoxel(x, y, z, Voxel(VoxelType::Cobblestone, QColor(128, 128, 128)));
                    } else {
                        chunk->setVoxel(x, y, z, Voxel(VoxelType::Dirt, QColor(139, 69, 19)));
                    }
                }
            }
        }
    }
    
    // Add some features based on random seed
    if (coordinate.getY() == 0) {
        // Trees or rocks
        std::uniform_int_distribution<int> featureDist(0, 10);
        std::uniform_int_distribution<int> positionDist(2, ChunkCoordinate::CHUNK_SIZE - 3);
        
        // Number of features depends on chunk position (consistent for the same chunk)
        int chunkHash = coordinate.getX() * 31 + coordinate.getZ() * 17;
        std::mt19937 chunkRandom(m_seed + chunkHash);
        
        int numFeatures = featureDist(chunkRandom) % 3;
        
        for (int i = 0; i < numFeatures; i++) {
            int featureX = positionDist(chunkRandom);
            int featureZ = positionDist(chunkRandom);
            
            // 50% chance for tree, 50% for rock
            bool isTree = featureDist(chunkRandom) > 5;
            
            if (isTree) {
                // Simple tree: wooden trunk and leaf top
                // Trunk
                for (int y = 0; y < 3; y++) {
                    chunk->setVoxel(featureX, 15 + y, featureZ, 
                                   Voxel(VoxelType::Solid, QColor(101, 67, 33)));
                }
                
                // Leaves (simple cube for now)
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = 0; dy <= 2; dy++) {
                        for (int dz = -1; dz <= 1; dz++) {
                            if (dx == 0 && dz == 0 && dy < 2) {
                                continue; // Trunk location, skip
                            }
                            
                            int lx = featureX + dx;
                            int ly = 18 + dy;
                            int lz = featureZ + dz;
                            
                            // Check boundaries
                            if (lx >= 0 && lx < ChunkCoordinate::CHUNK_SIZE &&
                                ly >= 0 && ly < ChunkCoordinate::CHUNK_SIZE &&
                                lz >= 0 && lz < ChunkCoordinate::CHUNK_SIZE) {
                                
                                chunk->setVoxel(lx, ly, lz, 
                                               Voxel(VoxelType::Solid, QColor(0, 100, 0)));
                            }
                        }
                    }
                }
            } else {
                // Simple rock
                chunk->setVoxel(featureX, 16, featureZ, 
                               Voxel(VoxelType::Cobblestone, QColor(128, 128, 128)));
                
                // 50% chance for larger rock
                if (featureDist(chunkRandom) > 5) {
                    chunk->setVoxel(featureX, 17, featureZ, 
                                   Voxel(VoxelType::Cobblestone, QColor(128, 128, 128)));
                }
            }
        }
    }
    
    // Optimize the chunk
    chunk->optimize();
    
    return chunk;
}

void FlatTerrainGenerator::setSeed(unsigned int seed) {
    m_seed = seed;
    m_random.seed(m_seed);
}

//-------------------------------------------------------------------------
// NoiseTerrainGenerator implementation
//-------------------------------------------------------------------------

// Simplex Noise implementation (simplified version)
namespace {
    float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }
    
    float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
    
    float simplex2D(float x, float y, unsigned int seed) {
        // Initialize permutation table based on seed
        const int tableSize = 256;
        int perm[tableSize * 2];
        
        std::mt19937 rng(seed);
        std::vector<int> p(tableSize);
        for (int i = 0; i < tableSize; i++) {
            p[i] = i;
        }
        std::shuffle(p.begin(), p.end(), rng);
        
        for (int i = 0; i < tableSize; i++) {
            perm[i] = perm[i + tableSize] = p[i];
        }
        
        // Find unit grid cell containing point
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        
        // Get relative coords of point within that cell
        x -= std::floor(x);
        y -= std::floor(y);
        
        // Compute fade curves for each coord
        float u = fade(x);
        float v = fade(y);
        
        // Hash coords of the 4 square corners
        int A = perm[X] + Y;
        int AA = perm[A];
        int AB = perm[A + 1];
        int B = perm[X + 1] + Y;
        int BA = perm[B];
        int BB = perm[B + 1];
        
        // Add blended results from 4 corners of the square
        float res = lerp(v, 
                        lerp(u, grad(perm[AA], x, y), 
                            grad(perm[BA], x-1, y)),
                        lerp(u, grad(perm[AB], x, y-1), 
                            grad(perm[BB], x-1, y-1)));
        
        // Return result in range [-1, 1]
        return res;
    }
}

NoiseTerrainGenerator::NoiseTerrainGenerator()
    : m_seed(0),
      m_frequency(0.01f),
      m_amplitude(32.0f),
      m_octaves(4),
      m_lacunarity(2.0f),
      m_persistence(0.5f) {
    // Initialize random generator with default seed
    m_random.seed(m_seed);
}

std::shared_ptr<Chunk> NoiseTerrainGenerator::generateChunk(const ChunkCoordinate& coordinate) {
    // Create a new chunk
    auto chunk = std::make_shared<Chunk>(coordinate);
    
    // Only generate terrain if the chunk is within terrain height range
    int chunkY = coordinate.getY();
    
    // Max height = amplitude * sum of geometric series = amplitude * (1-persistence^octaves)/(1-persistence)
    float maxHeight = m_amplitude * (1.0f - std::pow(m_persistence, m_octaves)) / (1.0f - m_persistence);
    
    // Convert to chunk coordinates (chunk height = 16 blocks)
    int maxChunkHeight = static_cast<int>(maxHeight / ChunkCoordinate::CHUNK_SIZE) + 1;
    
    // Skip if above or far below max height
    if (chunkY > maxChunkHeight || chunkY < -3) {
        return chunk;  // Empty chunk
    }
    
    // Generate terrain using noise
    for (int x = 0; x < ChunkCoordinate::CHUNK_SIZE; x++) {
        for (int z = 0; z < ChunkCoordinate::CHUNK_SIZE; z++) {
            // Convert to world coordinates
            float worldX = coordinate.getX() * ChunkCoordinate::CHUNK_SIZE + x;
            float worldZ = coordinate.getZ() * ChunkCoordinate::CHUNK_SIZE + z;
            
            // Get height from noise function
            float noiseHeight = getFractalNoise(worldX, worldZ);
            
            // Adjust range to 0 to maxHeight
            noiseHeight = (noiseHeight + 1.0f) * 0.5f * maxHeight;
            
            // Round to integer
            int height = static_cast<int>(noiseHeight);
            
            // Convert to block coordinates
            int blockHeight = height;
            int relativeY = blockHeight - (chunkY * ChunkCoordinate::CHUNK_SIZE);
            
            // Fill the chunk based on height
            for (int y = 0; y < ChunkCoordinate::CHUNK_SIZE; y++) {
                // Skip if this y level is above the terrain height
                if (y > relativeY) {
                    continue;
                }
                
                // Determine material based on depth from surface
                VoxelType voxelType;
                QColor voxelColor;
                
                if (y == relativeY) {
                    // Surface layer
                    if (blockHeight > m_amplitude * 0.7f) {
                        // Snow on mountains
                        voxelType = VoxelType::Solid;
                        voxelColor = QColor(240, 240, 255);
                    } else if (blockHeight > m_amplitude * 0.4f) {
                        // Rock on hills
                        voxelType = VoxelType::Cobblestone;
                        voxelColor = QColor(128, 128, 128);
                    } else {
                        // Grass on lowlands
                        voxelType = VoxelType::Grass;
                        voxelColor = QColor(34, 139, 34);
                    }
                } else if (y >= relativeY - 3) {
                    // Dirt layer below surface
                    voxelType = VoxelType::Dirt;
                    voxelColor = QColor(139, 69, 19);
                } else {
                    // Stone below dirt
                    voxelType = VoxelType::Cobblestone;
                    voxelColor = QColor(128, 128, 128);
                }
                
                // Set the voxel
                chunk->setVoxel(x, y, z, Voxel(voxelType, voxelColor));
            }
        }
    }
    
    // Optimize the chunk
    chunk->optimize();
    
    return chunk;
}

void NoiseTerrainGenerator::setSeed(unsigned int seed) {
    m_seed = seed;
    m_random.seed(m_seed);
}

float NoiseTerrainGenerator::getNoise(float x, float y) const {
    // Use the simplex noise function
    return simplex2D(x * m_frequency, y * m_frequency, m_seed);
}

float NoiseTerrainGenerator::getFractalNoise(float x, float y) const {
    float amplitude = 1.0f;
    float frequency = m_frequency;
    float noise = 0.0f;
    float max = 0.0f;
    
    // Add multiple octaves of noise
    for (int i = 0; i < m_octaves; i++) {
        noise += simplex2D(x * frequency, y * frequency, m_seed + i) * amplitude;
        max += amplitude;
        amplitude *= m_persistence;
        frequency *= m_lacunarity;
    }
    
    // Normalize
    return noise / max;
}

//-------------------------------------------------------------------------
// SphericalPlanetGenerator implementation
//-------------------------------------------------------------------------

SphericalPlanetGenerator::SphericalPlanetGenerator()
    : m_seed(0),
      m_radius(1000.0f),
      m_terrainHeight(50.0f),
      m_seaLevel(0.5f) {
    // Initialize random generator with default seed
    m_random.seed(m_seed);
    
    // Create noise generator for terrain
    m_noiseGenerator = std::make_unique<NoiseTerrainGenerator>();
    m_noiseGenerator->setSeed(m_seed);
    m_noiseGenerator->setFrequency(0.001f);  // Lower frequency for smoother terrain
    m_noiseGenerator->setOctaves(6);         // More octaves for detail
    m_noiseGenerator->setAmplitude(1.0f);    // We'll multiply by terrain height later
}

std::shared_ptr<Chunk> SphericalPlanetGenerator::generateChunk(const ChunkCoordinate& coordinate) {
    // Create a new chunk
    auto chunk = std::make_shared<Chunk>(coordinate);
    
    // Get world-space coordinates of chunk's minimum corner
    QVector3D chunkCorner = coordinate.getMinCorner();
    
    // Calculate approximate distance from planet center
    float distanceToCenter = chunkCorner.length();
    
    // Skip chunks that are far from the planet surface
    float surfaceRange = m_radius + m_terrainHeight * 2.0f;
    float innerRange = m_radius - m_terrainHeight * 2.0f;
    
    if (distanceToCenter > surfaceRange || distanceToCenter < innerRange * 0.5f) {
        // Empty chunk far from surface
        return chunk;
    }
    
    // Check each voxel in the chunk
    for (int x = 0; x < ChunkCoordinate::CHUNK_SIZE; x++) {
        for (int y = 0; y < ChunkCoordinate::CHUNK_SIZE; y++) {
            for (int z = 0; z < ChunkCoordinate::CHUNK_SIZE; z++) {
                // Get world position of this voxel
                QVector3D worldPos = coordinate.toWorldPosition(x, y, z);
                
                // Distance from center of planet
                float distance = worldPos.length();
                
                // Skip if far from surface
                if (distance > surfaceRange || distance < innerRange) {
                    continue;
                }
                
                // Convert to spherical coordinates
                float longitude, latitude, radius;
                cartesianToSpherical(worldPos, longitude, latitude, radius);
                
                // Get noise value for this point on the sphere
                float noiseValue = m_noiseGenerator->getFractalNoise(
                    longitude * m_radius, 
                    latitude * m_radius
                );
                
                // Scale noise to terrain height
                float heightOffset = noiseValue * m_terrainHeight;
                
                // Determine if this voxel is inside the planet
                bool isInside = radius < m_radius + heightOffset;
                
                if (isInside) {
                    // Create voxel at this position
                    VoxelType voxelType;
                    QColor voxelColor;
                    
                    // Determine material based on depth from surface
                    float depthFactor = (radius - innerRange) / (m_radius + heightOffset - innerRange);
                    
                    if (depthFactor > 0.95f) {
                        // Surface layer
                        if (heightOffset < m_terrainHeight * m_seaLevel) {
                            // Ocean
                            voxelType = VoxelType::Solid;
                            voxelColor = QColor(0, 119, 190);
                        } else if (heightOffset < m_terrainHeight * 0.7f) {
                            // Land
                            voxelType = VoxelType::Grass;
                            voxelColor = QColor(34, 139, 34);
                        } else {
                            // Mountains
                            voxelType = VoxelType::Cobblestone;
                            voxelColor = QColor(160, 160, 160);
                        }
                    } else if (depthFactor > 0.85f) {
                        // Dirt/sand under surface
                        voxelType = VoxelType::Dirt;
                        voxelColor = QColor(139, 69, 19);
                    } else if (depthFactor > 0.5f) {
                        // Stone
                        voxelType = VoxelType::Cobblestone;
                        voxelColor = QColor(128, 128, 128);
                    } else {
                        // Core
                        voxelType = VoxelType::Solid;
                        voxelColor = QColor(200, 50, 50);
                    }
                    
                    // Set the voxel
                    chunk->setVoxel(x, y, z, Voxel(voxelType, voxelColor));
                }
            }
        }
    }
    
    // Optimize the chunk
    chunk->optimize();
    
    return chunk;
}

void SphericalPlanetGenerator::setSeed(unsigned int seed) {
    m_seed = seed;
    m_random.seed(m_seed);
    
    // Update noise generator seed
    if (m_noiseGenerator) {
        m_noiseGenerator->setSeed(seed);
    }
}

QVector3D SphericalPlanetGenerator::sphericalToCartesian(float longitude, float latitude, float radius) const {
    float x = radius * cos(latitude) * cos(longitude);
    float y = radius * sin(latitude);
    float z = radius * cos(latitude) * sin(longitude);
    
    return QVector3D(x, y, z);
}

void SphericalPlanetGenerator::cartesianToSpherical(const QVector3D& pos, float& longitude, float& latitude, float& radius) const {
    radius = pos.length();
    
    if (radius < 0.0001f) {
        // Avoid division by zero
        longitude = 0.0f;
        latitude = 0.0f;
        return;
    }
    
    latitude = asin(pos.y() / radius);
    longitude = atan2(pos.z(), pos.x());
}

bool SphericalPlanetGenerator::isPointInSphere(const QVector3D& point, float radius) const {
    return point.lengthSquared() <= radius * radius;
}