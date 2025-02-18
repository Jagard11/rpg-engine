# ./GameV2/map_generator.py
# Generates a world map with biomes using Perlin noise, offset with narrow seam stitching

import noise
from biome_types import BIOME_TYPES, VALID_BIOMES
from logger import info, log_map_gen

class MapGenerator:
    def __init__(self, width, height, seed=None):
        self.width = width
        self.height = height
        self.seed = seed if seed is not None else 42
        self.tiles = None
        info(f"MapGenerator initialized: {width}x{height}, seed={self.seed}")

    def generate(self):
        continent_scale = 50.0
        detail_scale = 10.0
        octaves = 6
        persistence = 0.5
        lacunarity = 2.0

        # Step 1: Generate initial noise maps
        continent_noise = [[0 for _ in range(self.width)] for _ in range(self.height)]
        elevation = [[0 for _ in range(self.width)] for _ in range(self.height)]
        moisture = [[0 for _ in range(self.width)] for _ in range(self.height)]

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

        # Step 2: Offset by half width (200 tiles)
        offset = self.width // 2  # 200
        shifted_continent = [[0 for _ in range(self.width)] for _ in range(self.height)]
        shifted_elevation = [[0 for _ in range(self.width)] for _ in range(self.height)]
        shifted_moisture = [[0 for _ in range(self.width)] for _ in range(self.height)]

        for y in range(self.height):
            for x in range(self.width):
                new_x = (x + offset) % self.width
                shifted_continent[y][x] = continent_noise[y][new_x]
                shifted_elevation[y][x] = elevation[y][new_x]
                shifted_moisture[y][x] = moisture[y][new_x]

        # Step 3: Narrow seam stitch (x=198 to x=201)
        seam_center = offset - 1  # 199 (original seam after offset)
        blend_start = seam_center - 1  # 198
        blend_end = seam_center + 2    # 201
        blend_width = blend_end - blend_start  # 3 tiles

        for y in range(self.height):
            for x in range(blend_start, blend_end):
                t = (x - blend_start) / blend_width  # 0 to 1 over 3 tiles
                left_cont = shifted_continent[y][blend_start - 1]  # x=197
                right_cont = shifted_continent[y][blend_end]       # x=201
                left_elev = shifted_elevation[y][blend_start - 1]
                right_elev = shifted_elevation[y][blend_end]
                left_moist = shifted_moisture[y][blend_start - 1]
                right_moist = shifted_moisture[y][blend_end]

                # Linear blend across seam
                shifted_continent[y][x] = left_cont * (1 - t) + right_cont * t
                shifted_elevation[y][x] = left_elev * (1 - t) + right_elev * t
                shifted_moisture[y][x] = left_moist * (1 - t) + right_moist * t

        # Step 4: Normalize noise
        cont_min, cont_max = min(map(min, shifted_continent)), max(map(max, shifted_continent))
        elev_min, elev_max = min(map(min, shifted_elevation)), max(map(max, shifted_elevation))
        moist_min, moist_max = min(map(min, shifted_moisture)), max(map(max, shifted_moisture))

        for y in range(self.height):
            for x in range(self.width):
                shifted_continent[y][x] = (shifted_continent[y][x] - cont_min) / (cont_max - cont_min)
                shifted_elevation[y][x] = (shifted_elevation[y][x] - elev_min) / (elev_max - elev_min)
                shifted_moisture[y][x] = (shifted_moisture[y][x] - moist_min) / (moist_max - moist_min)

        # Debug seams
        for y in range(self.height):
            if abs(shifted_continent[y][0] - shifted_continent[y][self.width - 1]) > 0.0001:
                info(f"Edge seam mismatch at y={y}: {shifted_continent[y][0]} != {shifted_continent[y][self.width - 1]}")
            if abs(shifted_continent[y][blend_start - 1] - shifted_continent[y][blend_start]) > 0.05:
                info(f"Middle left seam mismatch at y={y}: {shifted_continent[y][blend_start - 1]} != {shifted_continent[y][blend_start]}")
            if abs(shifted_continent[y][blend_end - 1] - shifted_continent[y][blend_end]) > 0.05:
                info(f"Middle right seam mismatch at y={y}: {shifted_continent[y][blend_end - 1]} != {shifted_continent[y][blend_end]}")

        # Assign biomes
        self.tiles = [[None for _ in range(self.width)] for _ in range(self.height)]
        for y in range(self.height):
            for x in range(self.width):
                cont = shifted_continent[y][x]
                elev = shifted_elevation[y][x]
                moist = shifted_moisture[y][x]
                temp = 1.0 - abs((y / (self.height - 1)) - 0.5) * 2.0

                if cont < 0.4:
                    biome = "DEEP_WATER"
                elif cont < 0.45:
                    biome = "SHALLOW_WATER"
                elif cont < 0.5:
                    biome = "BEACH"
                else:
                    biome = self._assign_land_biome(elev, moist, temp)

                self.tiles[y][x] = biome

        # Check biome seams
        for y in range(self.height):
            if self.tiles[y][0] != self.tiles[y][self.width - 1]:
                info(f"Edge biome seam mismatch at y={y}: {self.tiles[y][0]} != {self.tiles[y][self.width - 1]}")
            if self.tiles[y][blend_start - 1] != self.tiles[y][blend_start]:
                info(f"Middle left biome seam mismatch at y={y}: {self.tiles[y][blend_start - 1]} != {self.tiles[y][blend_start]}")
            if self.tiles[y][blend_end - 1] != self.tiles[y][blend_end]:
                info(f"Middle right biome seam mismatch at y={y}: {self.tiles[y][blend_end - 1]} != {self.tiles[y][blend_end]}")

        info("World map generation complete")
        return self.tiles

    def _assign_land_biome(self, elevation, moisture, temperature):
        if elevation < 0.3:
            if temperature < 0.3:
                return "TUNDRA"
            elif moisture > 0.7 and temperature > 0.5:
                return "SWAMP"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        elif elevation < 0.7:
            if temperature < 0.3:
                return "TUNDRA"
            elif moisture > 0.6 and temperature > 0.4:
                return "FOREST"
            elif moisture < 0.3 and temperature > 0.7:
                return "DESERT"
            return "GRASSLAND"
        else:
            if temperature < 0.4:
                return "TUNDRA"
            elif moisture > 0.5 and temperature > 0.4:
                return "FOREST"
            return "MOUNTAIN"