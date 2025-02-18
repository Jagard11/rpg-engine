# ./GameV2/biome_types.py
# Defines biome tile types with environmental ranges

# Biome definitions with ranges for continentality, elevation, moisture, and temperature
BIOME_TYPES = {
    "DEEP_WATER": {"name": "Deep Water", "color": (0, 0, 139), "continent_range": [0.0, 0.4], "elev_range": [0.0, 1.0], "moist_range": [0.0, 1.0], "temp_range": [0.0, 1.0]},
    "SHALLOW_WATER": {"name": "Shallow Water", "color": (0, 191, 255), "continent_range": [0.4, 0.45], "elev_range": [0.0, 1.0], "moist_range": [0.0, 1.0], "temp_range": [0.0, 1.0]},
    "BEACH": {"name": "Beach", "color": (245, 222, 179), "continent_range": [0.45, 0.5], "elev_range": [0.0, 1.0], "moist_range": [0.0, 1.0], "temp_range": [0.0, 1.0]},
    "POLAR_DESERT": {"name": "Polar Desert", "color": (200, 220, 230), "continent_range": [0.5, 1.0], "elev_range": [0.0, 1.0], "moist_range": [0.0, 0.2], "temp_range": [0.0, 0.15]},
    "TUNDRA": {"name": "Tundra", "color": (173, 216, 230), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.8], "moist_range": [0.2, 0.5], "temp_range": [0.0, 0.2]},
    "ICE_CAP": {"name": "Ice Cap", "color": (240, 248, 255), "continent_range": [0.5, 1.0], "elev_range": [0.0, 1.0], "moist_range": [0.5, 1.0], "temp_range": [0.0, 0.15]},
    "TAIGA": {"name": "Taiga", "color": (95, 158, 160), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.6], "moist_range": [0.4, 0.7], "temp_range": [0.15, 0.3]},
    "STEPPE": {"name": "Steppe", "color": (154, 205, 50), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.4], "moist_range": [0.1, 0.4], "temp_range": [0.3, 0.6]},
    "GRASSLAND": {"name": "Grassland", "color": (124, 252, 0), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.4], "moist_range": [0.4, 0.7], "temp_range": [0.3, 0.7]},
    "WETLAND": {"name": "Wetland", "color": (107, 142, 35), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.2], "moist_range": [0.7, 1.0], "temp_range": [0.3, 0.5]},
    "SAVANNA": {"name": "Savanna", "color": (189, 183, 107), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.4], "moist_range": [0.2, 0.5], "temp_range": [0.6, 0.8]},
    "SWAMP": {"name": "Swamp", "color": (139, 69, 19), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.2], "moist_range": [0.7, 1.0], "temp_range": [0.5, 0.8]},
    "DESERT": {"name": "Desert", "color": (210, 180, 140), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.6], "moist_range": [0.0, 0.3], "temp_range": [0.7, 1.0]},
    "ARID_SCRUB": {"name": "Arid Scrub", "color": (169, 139, 87), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.6], "moist_range": [0.3, 0.5], "temp_range": [0.7, 1.0]},
    "JUNGLE": {"name": "Jungle", "color": (0, 100, 0), "continent_range": [0.5, 1.0], "elev_range": [0.0, 0.4], "moist_range": [0.7, 1.0], "temp_range": [0.7, 1.0]},
    "FOREST": {"name": "Forest", "color": (34, 139, 34), "continent_range": [0.5, 1.0], "elev_range": [0.2, 0.6], "moist_range": [0.5, 0.8], "temp_range": [0.4, 0.7]},
    "ALPINE_MEADOW": {"name": "Alpine Meadow", "color": (144, 238, 144), "continent_range": [0.5, 1.0], "elev_range": [0.6, 0.9], "moist_range": [0.4, 0.7], "temp_range": [0.2, 0.5]},
    "PLATEAU": {"name": "Plateau", "color": (139, 137, 137), "continent_range": [0.5, 1.0], "elev_range": [0.6, 0.9], "moist_range": [0.0, 0.5], "temp_range": [0.4, 0.7]},
    "MOUNTAIN": {"name": "Mountain", "color": (105, 105, 105), "continent_range": [0.5, 1.0], "elev_range": [0.8, 1.0], "moist_range": [0.0, 1.0], "temp_range": [0.0, 0.6]},
}

VALID_BIOMES = list(BIOME_TYPES.keys())