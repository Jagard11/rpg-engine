# tiles.py

from enum import Enum
from typing import Tuple

class TileType(Enum):
    OCEAN = 1
    PLAINS = 2
    FOREST = 3
    DESERT = 4
    MOUNTAIN = 5
    TUNDRA = 6

class Tile:
    def __init__(self, tile_type: TileType):
        self.type = tile_type

    @property
    def color(self) -> Tuple[int, int, int]:
        colors = {
            TileType.OCEAN: (30, 144, 255),
            TileType.PLAINS: (124, 252, 0),
            TileType.FOREST: (34, 139, 34),
            TileType.DESERT: (238, 214, 175),
            TileType.MOUNTAIN: (139, 137, 137),
            TileType.TUNDRA: (245, 245, 220)
        }
        return colors[self.type]

def get_tile_for_biome(biome: str) -> Tile:
    biome_to_tile = {
        'ocean': TileType.OCEAN,
        'plains': TileType.PLAINS,
        'forest': TileType.FOREST,
        'desert': TileType.DESERT,
        'mountain': TileType.MOUNTAIN,
        'tundra': TileType.TUNDRA
    }
    return Tile(biome_to_tile.get(biome, TileType.PLAINS))
