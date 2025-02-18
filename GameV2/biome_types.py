# ./GameV2/biome_types.py
# Defines biome tile types with names and colors for the world generation prototype

# Biome tile definitions as a dictionary
BIOME_TYPES = {
    "DEBUG": {
        "name": "Debug",
        "color": (255, 105, 180)  # Neon pink for visibility
    },
    "DEEP_WATER": {
        "name": "Deep Water",
        "color": (0, 0, 139)  # Dark blue
    },
    "SHALLOW_WATER": {
        "name": "Shallow Water",
        "color": (0, 191, 255)  # Light blue
    },
    "BEACH": {
        "name": "Beach",
        "color": (245, 222, 179)  # Sandy beige
    },
    "GRASSLAND": {
        "name": "Grassland",
        "color": (124, 252, 0)  # Bright green
    },
    "FOREST": {
        "name": "Forest",
        "color": (34, 139, 34)  # Dark green
    },
    "SWAMP": {
        "name": "Swamp",
        "color": (139, 69, 19)  # Murky brown
    },
    "DESERT": {
        "name": "Desert",
        "color": (210, 180, 140)  # Tan
    },
    "MOUNTAIN": {
        "name": "Mountain",
        "color": (105, 105, 105)  # Gray
    },
    "TUNDRA": {
        "name": "Tundra",
        "color": (173, 216, 230)  # Light blue-ish for cold plains
    }
}

# List of valid biome keys (excluding DEBUG) for generation purposes
VALID_BIOMES = [key for key in BIOME_TYPES.keys() if key != "DEBUG"]

if __name__ == "__main__":
    # Quick test to print all biome types when run directly
    for biome_key, biome_data in BIOME_TYPES.items():
        print(f"{biome_key}: {biome_data['name']} - Color: {biome_data['color']}")