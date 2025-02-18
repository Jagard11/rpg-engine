# ./GameV2/map_generator.py
# Generates a world map with biomes using Perlin noise for continents and details

import noise
from biome_types import BIOME_TYPES, VALID_BIOMES
from logger import info, log_map_gen

class MapGenerator:
    def __init__(self, width, height, seed=None):
        """Initialize the map generator with dimensions and optional seed."""
        self.width = width
        self.height = height
        self.seed = seed if seed is not None else 42  # Default seed for consistency
        self.tiles = None
        info(f"MapGenerator initialized: {width}x{height}, seed={self.seed}")

    def generate(self):
        """Generate a world map with continents, islands, and biomes."""
        # Noise parameters
        continent_scale = 50.0  # Large scale for continents
        detail_scale = 10.0     # Smaller scale for biome details
        octaves = 6
        persistence = 0.5
        lacunarity = 2.0

        # Generate noise maps
        continent_noise = [[0 for _ in range(self.width)] for _ in range(self.height)]
        elevation = [[0 for _ in range(self.width)] for _ in range(self.height)]
        moisture = [[0 for _ in range(self.width)] for _ in range(self.height)]

        for y in range(self.height):
            for x in range(self.width):
                # Continent noise (low frequency, seamless)
                continent_noise[y][x] = noise.pnoise2(
                    x / continent_scale, y / continent_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeatx=self.width, repeaty=self.height, base=self.seed
                )
                # Elevation noise (seamless)
                elevation[y][x] = noise.pnoise2(
                    x / detail_scale, y / detail_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeatx=self.width, repeaty=self.height, base=self.seed + 1
                )
                # Moisture noise (seamless)
                moisture[y][x] = noise.pnoise2(
                    x / detail_scale, y / detail_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeatx=self.width, repeaty=self.height, base=self.seed + 2
                )

        # Normalize noise values to 0-1 range
        cont_min, cont_max = min(map(min, continent_noise)), max(map(max, continent_noise))
        elev_min, elev_max = min(map(min, elevation)), max(map(max, elevation))
        moist_min, moist_max = min(map(min, moisture)), max(map(max, moisture))

        for y in range(self.height):
            for x in range(self.width):
                continent_noise[y][x] = (continent_noise[y][x] - cont_min) / (cont_max - cont_min)
                elevation[y][x] = (elevation[y][x] - elev_min) / (elev_max - elev_min)
                moisture[y][x] = (moisture[y][x] - moist_min) / (moist_max - moist_min)

        # Assign biomes with wrapping check
        self.tiles = [[None for _ in range(self.width)] for _ in range(self.height)]
        for y in range(self.height):
            for x in range(self.width):
                cont = continent_noise[y][x]
                elev = elevation[y][x]
                moist = moisture[y][x]
                temp = 1.0 - abs((y / (self.height - 1)) - 0.5) * 2.0

                # Define land vs water with islands
                if cont < 0.4:  # Ocean
                    biome = "DEEP_WATER"
                elif cont < 0.45:  # Shallow water
                    biome = "SHALLOW_WATER"
                elif cont < 0.5:  # Beaches
                    biome = "BEACH"
                else:  # Land
                    biome = self._assign_land_biome(elev, moist, temp)

                self.tiles[y][x] = BIOME_TYPES[biome]["color"]
                log_map_gen(f"Tile ({x}, {y}): cont={cont:.2f}, elev={elev:.2f}, moist={moist:.2f}, temp={temp:.2f}, biome={biome}")

        # Verify seam continuity (debug)
        for y in range(self.height):
            if self.tiles[y][0] != self.tiles[y][self.width - 1]:
                log_map_gen(f"Seam mismatch at y={y}: {self.tiles[y][0]} != {self.tiles[y][self.width - 1]}")

        info("World map generation complete")
        return self.tiles

    def _assign_land_biome(self, elevation, moisture, temperature):
        """Assign a biome for land tiles based on elevation, moisture, and temperature."""
        if elevation < 0.3:  # Lowland
            if temperature < 0.3:
                return "TUNDRA"
            elif moisture > 0.7 and temperature > 0.5:
                return "SWAMP"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        elif elevation < 0.7:  # Mid-level
            if temperature < 0.3:
                return "TUNDRA"
            elif moisture > 0.6 and temperature > 0.4:
                return "FOREST"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        else:  # High elevation
            if temperature < 0.4:
                return "TUNDRA"
            elif moisture > 0.5 and temperature > 0.4:
                return "FOREST"
            return "MOUNTAIN"