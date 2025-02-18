# ./GameV2/map_generator.py
# Generates a world map with biomes using Perlin noise, offset with narrow seam stitching

import noise
from biome_types import BIOME_TYPES, VALID_BIOMES
from logger import info, log_map_gen
import math

class MapGenerator:
    def __init__(self, width, height, seed=None, global_temp_modifier=0.1):
        self.width = width
        self.height = height
        self.seed = seed if seed is not None else 42
        self.global_temp_modifier = global_temp_modifier  # 0.001 to 1.0, shifts global temperature
        self.tiles = None
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

        for y in range(self.height):
            for x in range(self.width):
                shifted_continent[y][x] = (shifted_continent[y][x] - cont_min) / (cont_max - cont_min)
                shifted_elevation[y][x] = (shifted_elevation[y][x] - elev_min) / (elev_max - elev_min)
                shifted_moisture[y][x] = (shifted_moisture[y][x] - moist_min) / (moist_max - moist_min)
                shifted_temperature[y][x] = (shifted_temperature[y][x] - temp_min) / (temp_max - temp_min)

        for y in range(self.height):
            if abs(shifted_continent[y][0] - shifted_continent[y][self.width - 1]) > 0.0001:
                info(f"Edge seam mismatch at y={y}: {shifted_continent[y][0]} != {shifted_continent[y][self.width - 1]}")
            if abs(shifted_continent[y][blend_start - 1] - shifted_continent[y][blend_start]) > 0.05:
                info(f"Middle left seam mismatch at y={y}: {shifted_continent[y][blend_start - 1]} != {shifted_continent[y][blend_start]}")
            if abs(shifted_continent[y][blend_end - 1] - shifted_continent[y][blend_end]) > 0.05:
                info(f"Middle right seam mismatch at y={y}: {shifted_continent[y][blend_end - 1]} != {shifted_continent[y][blend_end]}")

        self.tiles = [[0 for _ in range(self.width)] for _ in range(self.height)]
        for y in range(self.height):
            for x in range(self.width):
                cont = shifted_continent[y][x]
                elev = shifted_elevation[y][x]
                moist = shifted_moisture[y][x]
                y_normalized = y / (self.height - 1)  # 0 to 1
                # Temperature: global shift with increased sensitivity at low values
                distance_from_equator = abs(y_normalized - 0.5) * 2.0  # 0 at equator, 1 at poles
                base_temp = 1.0 - distance_from_equator  # 1 at equator, 0 at poles
                temp_noise = shifted_temperature[y][x] * 0.1  # Noise ±0.1
                # Stronger shift for low values, normal for high
                temp_shift = self.global_temp_modifier * 10.0 if self.global_temp_modifier < 0.1 else self.global_temp_modifier * 1.2
                temp = base_temp + temp_noise - temp_shift
                temp = max(0.0, min(1.0, temp))

                if cont < 0.4:
                    biome = "DEEP_WATER"
                elif cont < 0.45:
                    biome = "SHALLOW_WATER"
                elif cont < 0.5:
                    biome = "BEACH"
                else:
                    biome = self._assign_land_biome(elev, moist, temp)

                self.tiles[y][x] = biome

        for y in range(self.height):
            if self.tiles[y][0] != self.tiles[y][self.width - 1]:
                info(f"Edge biome seam mismatch at y={y}: {self.tiles[y][0]} != {self.tiles[y][self.width - 1]}")
            if self.tiles[y][blend_start - 1] != self.tiles[y][blend_start]:
                info(f"Middle left biome seam mismatch at y={y}: {self.tiles[y][blend_start - 1]} != {self.tiles[y][blend_start]}")
            if self.tiles[y][blend_end - 1] != self.tiles[y][blend_end]:
                info(f"Middle right seam mismatch at y={y}: {self.tiles[y][blend_end - 1]} != {self.tiles[y][blend_end]}")

        info("World map generation complete")
        return self.tiles

    def _assign_land_biome(self, elevation, moisture, temperature):
        if temperature < 0.2:  # TUNDRA
            return "TUNDRA"
        elif temperature < 0.3:  # TAIGA
            return "TAIGA"
        elif elevation < 0.3:
            if moisture > 0.7 and temperature > 0.5:
                return "SWAMP"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        elif elevation < 0.7:
            if moisture > 0.6 and temperature > 0.4:
                return "FOREST"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        else:
            if moisture > 0.5 and temperature > 0.4:
                return "FOREST"
            return "MOUNTAIN"