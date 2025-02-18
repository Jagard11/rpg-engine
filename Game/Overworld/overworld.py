# ./Game/Overworld/overworld.py

import pygame
import numpy as np
from numpy.random import default_rng
import opensimplex
from math import sin, cos, pi, ceil
from datetime import datetime, timedelta
from tiles import get_tile_for_biome, Tile
from custom_logging import log, log_function_call, log_object_creation, log_method_call

# Initialize core systems
log('INFO', "Initializing Pygame")
pygame.init()

# Window/Viewport Configuration
WINDOW_WIDTH = 640
WINDOW_HEIGHT = 365

# World Configuration
WORLD_WIDTH = 2048  # Much larger than window
WORLD_HEIGHT = 1024  # Much larger than window
CHUNK_SIZE = 32
PLANET_RADIUS = 1000

# Calculate total chunks in world
WORLD_CHUNKS_X = ceil(WORLD_WIDTH / CHUNK_SIZE)
WORLD_CHUNKS_Y = ceil(WORLD_HEIGHT / CHUNK_SIZE)

# Initial view configuration
VIEWPORT_SCALE = 1.0  # Will be used for zoom later
VIEWPORT_X = 0  # Viewport's top-left position in world coordinates
VIEWPORT_Y = 0

# RNG setup
rng = default_rng()
SEED = rng.integers(2**32)

# Debug color
PLACEHOLDER_COLOR = (255, 0, 255)  # Bright pink RGB

log('INFO', f"Configuration set: Window={WINDOW_WIDTH}x{WINDOW_HEIGHT}, World={WORLD_WIDTH}x{WORLD_HEIGHT}")

class PlanetGenerator:
    def __init__(self, seed):
        log_object_creation('PlanetGenerator', seed=seed)
        self.noise = opensimplex.OpenSimplex(seed)
        self.chunk_cache = {}
        self.biome_params = {
            'ocean': (0.2, 0.8, 0.1),
            'forest': (0.5, 0.6, 0.4),
            'desert': (0.8, 0.2, 0.3),
            'tundra': (0.2, 0.3, 0.6),
            'mountain': (0.4, 0.5, 0.9)
        }

    def normalize_coordinates(self, x, y):
        """Convert world coordinates to normalized spherical coordinates."""
        lon = (x / WORLD_WIDTH) * 2 * pi
        lat = (y / WORLD_HEIGHT) * pi
        lon = lon % (2 * pi)
        lat = max(min(lat, pi/2), -pi/2)
        return lon, lat

    def generate_chunk(self, chunk_x, chunk_y):
        log_method_call('PlanetGenerator', 'generate_chunk', chunk_x=chunk_x, chunk_y=chunk_y)
        key = (chunk_x, chunk_y)
        if key in self.chunk_cache:
            return self.chunk_cache[key]

        chunk = []
        for y in range(CHUNK_SIZE):
            for x in range(CHUNK_SIZE):
                world_x = chunk_x * CHUNK_SIZE + x
                world_y = chunk_y * CHUNK_SIZE + y
                
                # Wrap coordinates for seamless world
                world_x = world_x % WORLD_WIDTH
                world_y = world_y % WORLD_HEIGHT
                
                lon, lat = self.normalize_coordinates(world_x, world_y)
                nx = cos(lat) * cos(lon)
                ny = cos(lat) * sin(lon)
                nz = sin(lat)
                
                elevation = (
                    0.5 * self.noise.noise3(nx, ny, nz) +
                    0.25 * self.noise.noise3(2*nx, 2*ny, 2*nz) +
                    0.125 * self.noise.noise3(4*nx, 4*ny, 4*nz)
                )
                
                temp = cos(lat) + 0.3 * self.noise.noise2(lon + 1000, lat + 1000)
                humidity = 0.5 * (
                    self.noise.noise2(lon + 2000, lat + 2000) +
                    self.noise.noise2(lon + 3000, lat + 3000)
                )
                
                biome = self._determine_biome(elevation, temp, humidity)
                tile = get_tile_for_biome(biome)
                chunk.append(tile)
        
        self.chunk_cache[key] = chunk
        return chunk

    def _determine_biome(self, elevation, temp, humidity):
        best_biome = 'plains'
        min_dist = float('inf')
        for biome, params in self.biome_params.items():
            dist = abs(params[0] - temp) + abs(params[1] - humidity) + abs(params[2] - elevation)
            if dist < min_dist:
                min_dist = dist
                best_biome = biome
        return best_biome

class WorldRenderer:
    def __init__(self, planet_gen):
        log_object_creation('WorldRenderer', planet_gen=planet_gen)
        self.planet_gen = planet_gen
        
        # Calculate visible chunks based on viewport
        self.visible_chunks_x = ceil(WINDOW_WIDTH / CHUNK_SIZE) + 1
        self.visible_chunks_y = ceil(WINDOW_HEIGHT / CHUNK_SIZE) + 1
        
        log('DEBUG', f"Visible chunks: {self.visible_chunks_x}x{self.visible_chunks_y}")

    def world_to_screen(self, world_x, world_y):
        """Convert world coordinates to screen coordinates."""
        screen_x = (world_x - VIEWPORT_X) * VIEWPORT_SCALE
        screen_y = (world_y - VIEWPORT_Y) * VIEWPORT_SCALE
        return int(screen_x), int(screen_y)

    def render_chunk(self, surface, chunk_x, chunk_y):
        try:
            chunk = self.planet_gen.generate_chunk(chunk_x, chunk_y)
            world_base_x = chunk_x * CHUNK_SIZE
            world_base_y = chunk_y * CHUNK_SIZE
            
            for y in range(CHUNK_SIZE):
                world_y = world_base_y + y
                
                for x in range(CHUNK_SIZE):
                    world_x = world_base_x + x
                    screen_x, screen_y = self.world_to_screen(world_x, world_y)
                    
                    if 0 <= screen_x < WINDOW_WIDTH and 0 <= screen_y < WINDOW_HEIGHT:
                        tile = chunk[y * CHUNK_SIZE + x]
                        surface.set_at((screen_x, screen_y), tile.color)
            
            return True
        except Exception as e:
            log('ERROR', f"Error rendering chunk: {str(e)}")
            return False

    def render_world(self, screen):
        log('INFO', "Starting world rendering")
        
        # Initial loading screen
        screen.fill((0, 0, 0))
        font = pygame.font.Font(None, 36)
        loading_text = font.render("Preparing to generate world...", True, (255, 255, 255))
        text_rect = loading_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2))
        screen.blit(loading_text, text_rect)
        pygame.display.flip()
        
        # Create world surface
        world_surface = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT))
        world_surface.fill(PLACEHOLDER_COLOR)
        
        # Calculate visible chunk range
        start_chunk_x = VIEWPORT_X // CHUNK_SIZE
        start_chunk_y = VIEWPORT_Y // CHUNK_SIZE
        chunks_x = ceil(WINDOW_WIDTH / (CHUNK_SIZE * VIEWPORT_SCALE))
        chunks_y = ceil(WINDOW_HEIGHT / (CHUNK_SIZE * VIEWPORT_SCALE))
        
        total_chunks = chunks_x * chunks_y
        chunks_generated = 0
        
        try:
            for cy in range(int(start_chunk_y), int(start_chunk_y + chunks_y + 1)):
                for cx in range(int(start_chunk_x), int(start_chunk_x + chunks_x + 1)):
                    chunks_generated += 1
                    progress = chunks_generated / total_chunks
                    
                    # Render chunk
                    success = self.render_chunk(world_surface, cx, cy)
                    if not success:
                        log('ERROR', f"Failed to render chunk at ({cx}, {cy})")
                        continue
                    
                    # Update progress display
                    screen.fill((0, 0, 0))
                    progress_text = f"Generating world... {int(progress * 100)}%"
                    text_surface = font.render(progress_text, True, (255, 255, 255))
                    text_rect = text_surface.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2))
                    screen.blit(text_surface, text_rect)
                    pygame.display.flip()
                    
                    # Handle events
                    for event in pygame.event.get():
                        if event.type == pygame.QUIT:
                            return False
            
            # Final render
            log('INFO', "World generation complete, displaying final render")
            screen.blit(world_surface, (0, 0))
            pygame.display.flip()
            return True
            
        except Exception as e:
            log('ERROR', f"Error during world rendering: {str(e)}")
            return False

def main():
    try:
        # Setup display
        log('INFO', "Setting up display")
        screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("World Generator")

        # Initialize systems
        planet_gen = PlanetGenerator(SEED)
        renderer = WorldRenderer(planet_gen)

        # Generate and display world
        success = renderer.render_world(screen)
        if not success:
            log('ERROR', "World generation failed")
            return

        # Keep window open
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

        pygame.quit()
        
    except Exception as e:
        log('CRITICAL', f"Fatal error in main: {str(e)}")
        pygame.quit()

if __name__ == "__main__":
    main()