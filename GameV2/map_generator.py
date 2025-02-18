# ./GameV2/map_generator.py
# Generates a world map with biomes using Perlin noise and chunk-based structure

import noise
from biome_types import BIOME_TYPES, VALID_BIOMES
from logger import info, log_map_gen
import math

class MapGenerator:
    def __init__(self, width, height, seed=None, global_temp_modifier=0.1):
        self.width = width
        self.height = height
        self.seed = seed if seed is not None else 42
        self.global_temp_modifier = global_temp_modifier
        self.tiles = None
        self.chunks = []  # Chunk grid
        info(f"MapGenerator initialized: {width}x{height}, seed={self.seed}, global_temp_modifier={self.global_temp_modifier}")

    def generate(self):
        continent_scale = 50.0
        detail_scale = 10.0
        temperature_scale = 20.0
        octaves = 6
        persistence = 0.5
        lacunarity = 2.0

        continent_noise = [[0 for _ in range(self.width)] for _ in range(self.height)]
        elevation = [[0 for _ in range(self.width)] for _ in range(self.height)]
        moisture = [[0 for _ in range(self.width)] for _ in range(self.height)]
        temperature_noise = [[0 for _ in range(self.width)] for _ in range(self.height)]

        for y in range(self.height):
            for x in range(self.width):
                continent_noise[y][x] = noise.pnoise2(
                    x / continent_scale, y / continent_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeaty=self.height, base=self.seed
                )
                elevation[y][x] = noise.pnoise2(
                    x / detail_scale, y / detail_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeaty=self.height, base=self.seed + 1
                )
                moisture[y][x] = noise.pnoise2(
                    x / detail_scale, y / detail_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeaty=self.height, base=self.seed + 2
                )
                temperature_noise[y][x] = noise.pnoise2(
                    x / temperature_scale, y / temperature_scale, octaves=octaves,
                    persistence=persistence, lacunarity=lacunarity,
                    repeaty=self.height, base=self.seed + 3
                )

        offset = self.width // 2
        shifted_continent = [[0 for _ in range(self.width)] for _ in range(self.height)]
        shifted_elevation = [[0 for _ in range(self.width)] for _ in range(self.height)]
        shifted_moisture = [[0 for _ in range(self.width)] for _ in range(self.height)]
        shifted_temperature = [[0 for _ in range(self.width)] for _ in range(self.height)]

        for y in range(self.height):
            for x in range(self.width):
                new_x = (x + offset) % self.width
                shifted_continent[y][x] = continent_noise[y][new_x]
                shifted_elevation[y][x] = elevation[y][new_x]
                shifted_moisture[y][x] = moisture[y][new_x]
                shifted_temperature[y][x] = temperature_noise[y][new_x]

        seam_center = offset - 1
        blend_start = seam_center - 1
        blend_end = seam_center + 2
        blend_width = blend_end - blend_start

        for y in range(self.height):
            for x in range(blend_start, blend_end):
                t = (x - blend_start) / blend_width
                left_cont = shifted_continent[y][blend_start - 1]
                right_cont = shifted_continent[y][blend_end]
                left_elev = shifted_elevation[y][blend_start - 1]
                right_elev = shifted_elevation[y][blend_end]
                left_moist = shifted_moisture[y][blend_start - 1]
                right_moist = shifted_moisture[y][blend_end]
                left_temp = shifted_temperature[y][blend_start - 1]
                right_temp = shifted_temperature[y][blend_end]

                shifted_continent[y][x] = left_cont * (1 - t) + right_cont * t
                shifted_elevation[y][x] = left_elev * (1 - t) + right_elev * t
                shifted_moisture[y][x] = left_moist * (1 - t) + right_moist * t
                shifted_temperature[y][x] = left_temp * (1 - t) + right_temp * t

        cont_min, cont_max = min(map(min, shifted_continent)), max(map(max, shifted_continent))
        elev_min, elev_max = min(map(min, shifted_elevation)), max(map(max, shifted_elevation))
        moist_min, moist_max = min(map(min, shifted_moisture)), max(map(max, shifted_moisture))
        temp_min, temp_max = min(map(min, shifted_temperature)), max(map(max, shifted_temperature))

        # Fix ZeroDivisionError by checking for zero range
        cont_range = cont_max - cont_min if cont_max != cont_min else 1e-10
        elev_range = elev_max - elev_min if elev_max != elev_min else 1e-10
        moist_range = moist_max - moist_min if moist_max != moist_min else 1e-10
        temp_range = temp_max - temp_min if temp_max != temp_min else 1e-10

        for y in range(self.height):
            for x in range(self.width):
                shifted_continent[y][x] = (shifted_continent[y][x] - cont_min) / cont_range
                shifted_elevation[y][x] = (shifted_elevation[y][x] - elev_min) / elev_range
                shifted_moisture[y][x] = (shifted_moisture[y][x] - moist_min) / moist_range
                shifted_temperature[y][x] = (shifted_temperature[y][x] - temp_min) / temp_range

        for y in range(self.height):
            if abs(shifted_continent[y][0] - shifted_continent[y][self.width - 1]) > 0.0001:
                info(f"Edge seam mismatch at y={y}: {shifted_continent[y][0]} != {shifted_continent[y][self.width - 1]}")
            if abs(shifted_continent[y][blend_start - 1] - shifted_continent[y][blend_start]) > 0.05:
                info(f"Middle left seam mismatch at y={y}: {shifted_continent[y][blend_start - 1]} != {shifted_continent[y][blend_start]}")
            if abs(shifted_continent[y][blend_end - 1] - shifted_continent[y][blend_end]) > 0.05:
                info(f"Middle right seam mismatch at y={y}: {shifted_continent[y][blend_end - 1]} != {shifted_continent[y][blend_end]}")

        # Generate tiles as Tile objects (fixed unpacking)
        self.tiles = [[Tile(cont, elev, moist, temp) 
                       for x, (cont, elev, moist, temp) in enumerate(zip(row_cont, row_elev, row_moist, row_temp))] 
                      for row_cont, row_elev, row_moist, row_temp in zip(shifted_continent, shifted_elevation, shifted_moisture, shifted_temperature)]

        # Initialize chunks (16x16)
        chunk_size = 16
        for cy in range(0, self.height, chunk_size):
            for cx in range(0, self.width, chunk_size):
                chunk_tiles = [self.tiles[y][x] for y in range(cy, min(cy + chunk_size, self.height))
                                             for x in range(cx, min(cx + chunk_size, self.width))]
                self.chunks.append(Chunk(cx // chunk_size, cy // chunk_size, chunk_tiles))

        # Initial biome assignment
        for chunk in self.chunks:
            update_chunk_tiles(chunk)

        info("World map generation complete")
        return self.tiles

class Tile:
    def __init__(self, continent, elevation, moisture, temperature):
        self.continent = continent
        self.elevation = elevation
        self.base_moisture = moisture
        self.current_moisture = moisture
        self.base_temp = temperature
        self.current_temp = temperature
        self.vegetation_level = 1.0
        self.biome = None  # Will be set by assign_biome

class Chunk:
    def __init__(self, x, y, tiles):
        self.x, self.y = x, y
        self.tiles = tiles
        self.avg_elev = sum(t.elevation for t in tiles) / len(tiles)
        self.base_moist = sum(t.base_moisture for t in tiles) / len(tiles)
        self.base_temp = sum(t.base_temp for t in tiles) / len(tiles)
        self.current_moist = self.base_moist
        self.current_temp = self.base_temp
        self.last_season_factor = 0
        self.last_temp = self.current_temp
        self.weather_state = "Clear"
        self.biome_dist = {}
        self.needs_update = True

    def update_season(self, season_factor):
        if abs(season_factor - self.last_season_factor) < 0.05:
            self.needs_update = False
            return
        self.last_season_factor = season_factor
        new_temp = self.base_temp + season_factor * 0.2 - temp_shift  # temp_shift from world_display
        new_moist = self.base_moist + season_factor * 0.1
        if (abs(new_temp - self.current_temp) > 0.1 or 
            abs(new_moist - self.current_moist) > 0.1):
            self.current_temp = new_temp
            self.current_moist = new_moist
            self.needs_update = True
        else:
            self.needs_update = False

class AlphaMap:
    def __init__(self, size, cycle_time):
        self.size = size
        self.cycle_time = cycle_time
        self.values = [self._compute_value(i / size) for i in range(size)]
        self.position = 0

    def _compute_value(self, t):
        # Day-night: 0–1 sinusoidal, Seasonal: -1 to 1 linear
        return math.sin(2 * math.pi * t) if self.cycle_time == 24 else 1 - 2 * t

    def update(self, game_time):
        self.position = (game_time / self.cycle_time) % 1.0 * self.size

    def get_value(self, coord):
        index = int(coord + self.position) % self.size
        return self.values[index]

def update_chunk_tiles(chunk):
    """Update tile biomes based on chunk conditions."""
    for tile in chunk.tiles:
        tile.current_temp = chunk.current_temp  # Simplified, could interpolate
        tile.current_moist = chunk.current_moist
        new_biome = assign_biome(tile.continent, tile.elevation, tile.current_moist, tile.current_temp)
        if new_biome != tile.biome:
            tile.biome = new_biome
    chunk.biome_dist = recalculate_biome_dist(chunk.tiles)

def assign_biome(continent, elevation, moisture, temperature):
    """Assign biome based on environmental ranges."""
    best_biome = None
    best_score = float('inf')

    adjusted_temp = temperature - (elevation * 0.5)  # Lapse rate
    adjusted_temp = max(0.0, min(1.0, adjusted_temp))

    for biome, data in BIOME_TYPES.items():
        if (data["continent_range"][0] <= continent <= data["continent_range"][1] and
            data["elev_range"][0] <= elevation <= data["elev_range"][1] and
            data["moist_range"][0] <= moisture <= data["moist_range"][1] and
            data["temp_range"][0] <= adjusted_temp <= data["temp_range"][1]):
            cont_center = (data["continent_range"][0] + data["continent_range"][1]) / 2
            elev_center = (data["elev_range"][0] + data["elev_range"][1]) / 2
            moist_center = (data["moist_range"][0] + data["moist_range"][1]) / 2
            temp_center = (data["temp_range"][0] + data["temp_range"][1]) / 2
            score = ((continent - cont_center) ** 2 + 
                     (elevation - elev_center) ** 2 + 
                     (moisture - moist_center) ** 2 + 
                     (adjusted_temp - temp_center) ** 2)
            if score < best_score:
                best_score = score
                best_biome = biome

    return best_biome if best_biome else "GRASSLAND"  # Fallback

def recalculate_biome_dist(tiles):
    """Recalculate biome distribution within a chunk."""
    biome_counts = {}
    total_tiles = len(tiles)
    for tile in tiles:
        biome_counts[tile.biome] = biome_counts.get(tile.biome, 0) + 1
    return {biome: count / total_tiles for biome, count in biome_counts.items()}

# Global temp_shift placeholder (set in world_display.py)
temp_shift = 0