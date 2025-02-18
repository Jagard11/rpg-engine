# ./GameV2/map_generator.py
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
        self.chunks = []
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

        self.tiles = [[Tile(cont, elev, moist, temp) 
                       for x, (cont, elev, moist, temp) in enumerate(zip(row_cont, row_elev, row_moist, row_temp))] 
                      for row_cont, row_elev, row_moist, row_temp in zip(shifted_continent, shifted_elevation, shifted_moisture, shifted_temperature)]

        # Initial biome assignment
        for y in range(self.height):
            for x in range(self.width):
                tile = self.tiles[y][x]
                tile.biome = assign_biome(tile.continent, tile.elevation, tile.current_moisture, tile.current_temp)  # Fixed typo here

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
        self.biome = None

def assign_biome(continent, elevation, moisture, temperature):
    best_biome = None
    best_score = float('inf')
    adjusted_temp = temperature - (elevation * 0.5)
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

    return best_biome if best_biome else "GRASSLAND"