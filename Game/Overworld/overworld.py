import pygame
import numpy as np
from numpy.random import default_rng
import opensimplex
from math import sin, cos, pi
from datetime import datetime, timedelta
from tiles import get_tile_for_biome, Tile
from custom_logging import log, log_function_call, log_object_creation, log_method_call

# Initialize core systems
log('INFO', "Initializing Pygame")
pygame.init()
clock = pygame.time.Clock()

# Configuration
WIDTH, HEIGHT = 640, 365
PLANET_RADIUS = 1000
rng = default_rng()
SEED = rng.integers(2**32)
TIME_SCALE = 1  # 1 real second = 1000 game seconds
CHUNK_RENDER_RADIUS = 2  # This will render a 5x5 grid of chunks
CHUNK_SIZE = 32  # Reduced from 64 to 32

log('INFO', f"Configuration set: WIDTH={WIDTH}, HEIGHT={HEIGHT}, SEED={SEED}")

class PlanetGenerator:
    def __init__(self, seed):
        log_object_creation('PlanetGenerator', seed=seed)
        self.noise = opensimplex.OpenSimplex(seed)
        self.biome_map = {}
        self.chunk_cache = {}
        
        self.biome_params = {
            'ocean': (0.2, 0.8, 0.1),
            'forest': (0.5, 0.6, 0.4),
            'desert': (0.8, 0.2, 0.3),
            'tundra': (0.2, 0.3, 0.6),
            'mountain': (0.4, 0.5, 0.9)
        }

    def generate_chunk(self, chunk_x, chunk_y):
        log_method_call('PlanetGenerator', 'generate_chunk', chunk_x=chunk_x, chunk_y=chunk_y)
        chunk_key = (chunk_x, chunk_y)
        if chunk_key in self.chunk_cache:
            return self.chunk_cache[chunk_key]

        chunk = []
        for y in range(CHUNK_SIZE):
            for x in range(CHUNK_SIZE):
                nx = (chunk_x * CHUNK_SIZE + x) / PLANET_RADIUS
                ny = (chunk_y * CHUNK_SIZE + y) / PLANET_RADIUS
                
                elevation = (
                    0.5 * self.noise.noise2(1 * nx, 1 * ny) +
                    0.25 * self.noise.noise2(2 * nx, 2 * ny) +
                    0.125 * self.noise.noise2(4 * nx, 4 * ny)
                )
                
                temp = self.noise.noise2(nx + 1000, ny + 1000)
                humidity = self.noise.noise2(nx + 2000, ny + 2000)
                
                biome = self._determine_biome(elevation, temp, humidity)
                chunk.append(get_tile_for_biome(biome))
        
        self.chunk_cache[chunk_key] = chunk
        return chunk

    def _determine_biome(self, elevation, temp, humidity):
        log_method_call('PlanetGenerator', '_determine_biome', elevation=elevation, temp=temp, humidity=humidity)
        best_biome = 'plains'
        min_dist = float('inf')
        
        for biome, params in self.biome_params.items():
            dist = abs(params[0]-temp) + abs(params[1]-humidity) + abs(params[2]-elevation)
            if dist < min_dist:
                min_dist = dist
                best_biome = biome
        log('DEBUG', f"Determined biome: {best_biome}")
        return best_biome

class TimeSystem:
    def __init__(self):
        log_object_creation('TimeSystem')
        self.start_time = datetime.now()
        self.game_time = self.start_time
        self.season_progress = 0  # 0-365 days
        
        self.axial_tilt = 23.4  # Degrees
        self.day_length = 24    # Hours
        self.year_length = 365  # Days

    def update(self, dt):
        log_method_call('TimeSystem', 'update', dt=dt)
        self.game_time += timedelta(seconds=dt*TIME_SCALE)
        self.season_progress = (self.season_progress + dt/TIME_SCALE) % 365
        
        season_angle = 2*pi * self.season_progress/365
        self.sun_altitude = sin(season_angle) * np.deg2rad(self.axial_tilt)
        self.sun_azimuth = (self.game_time.hour/24) * 2*pi
        log('DEBUG', f"Updated time: {self.game_time}, Season progress: {self.season_progress:.2f}")

    def get_light_level(self):
        light_level = max(0, sin(self.sun_altitude))
        log('DEBUG', f"Current light level: {light_level:.2f}")
        return light_level

    def get_season(self):
        season = 'winter'
        if 79 <= self.season_progress < 172: season = 'spring'
        elif 172 <= self.season_progress < 265: season = 'summer'
        elif 265 <= self.season_progress < 355: season = 'autumn'
        log('DEBUG', f"Current season: {season}")
        return season

class PlanetRenderer:
    def __init__(self, planet_gen):
        log_object_creation('PlanetRenderer', planet_gen=planet_gen)
        self.planet_gen = planet_gen
        self.loaded_chunks = {}

    def render_chunk(self, surface, chunk_x, chunk_y):
        log_method_call('PlanetRenderer', 'render_chunk', chunk_x=chunk_x, chunk_y=chunk_y)
        chunk_key = (chunk_x, chunk_y)
        if chunk_key not in self.loaded_chunks:
            log('DEBUG', f"Generating new chunk at {chunk_key}")
            self.loaded_chunks[chunk_key] = self.planet_gen.generate_chunk(chunk_x, chunk_y)
            
        chunk_data = self.loaded_chunks[chunk_key]
        for y in range(CHUNK_SIZE):
            for x in range(CHUNK_SIZE):
                tile = chunk_data[y * CHUNK_SIZE + x]
                color = tile.color
                pixel_x = (chunk_x * CHUNK_SIZE + x) % WIDTH
                pixel_y = (chunk_y * CHUNK_SIZE + y) % HEIGHT
                surface.set_at((pixel_x, pixel_y), color)
        log('DEBUG', f"Chunk {chunk_key} rendered")

    def render_world(self, surface):
        log_method_call('PlanetRenderer', 'render_world')
        for chunk_x in range(-CHUNK_RENDER_RADIUS, CHUNK_RENDER_RADIUS + 1):
            for chunk_y in range(-CHUNK_RENDER_RADIUS, CHUNK_RENDER_RADIUS + 1):
                self.render_chunk(surface, chunk_x, chunk_y)
        log('INFO', "World rendering complete")

# Initialize systems
log('INFO', "Initializing game systems")
planet_gen = PlanetGenerator(SEED)
time_system = TimeSystem()
renderer = PlanetRenderer(planet_gen)

# Main game loop
log('INFO', "Setting up display")
screen = pygame.display.set_mode((WIDTH, HEIGHT))
running = True

log('INFO', "Starting initial world generation")
screen.fill((0, 0, 0))

# Progressive world generation
total_chunks = (2 * CHUNK_RENDER_RADIUS + 1) ** 2
chunks_generated = 0
for chunk_x in range(-CHUNK_RENDER_RADIUS, CHUNK_RENDER_RADIUS + 1):
    for chunk_y in range(-CHUNK_RENDER_RADIUS, CHUNK_RENDER_RADIUS + 1):
        renderer.render_chunk(screen, chunk_x, chunk_y)
        chunks_generated += 1
        
        # Update progress
        progress = chunks_generated / total_chunks
        loading_text = f"Generating world... {int(progress * 100)}%"
        font = pygame.font.Font(None, 36)
        text_surface = font.render(loading_text, True, (255, 255, 255))
        text_rect = text_surface.get_rect(center=(WIDTH // 2, HEIGHT // 2))
        
        screen.fill((0, 0, 0))
        screen.blit(text_surface, text_rect)
        pygame.display.flip()
        
        # Handle events to keep the window responsive
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                exit()

pygame.display.flip()
log('INFO', "Initial world generation complete")

log('INFO', "Entering main game loop")
frame_count = 0
while running:
    dt = clock.tick(60)/1000
    
    time_system.update(dt)
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
            log('INFO', "Quit event received")
    
    # Redraw the world every 60 frames (about 1 second)
    if frame_count % 60 == 0:
        screen.fill((0, 0, 0))  # Clear the screen
        renderer.render_world(screen)
    
    # Apply lighting
    light_level = time_system.get_light_level()
    darken = pygame.Surface((WIDTH, HEIGHT))
    darken.fill((0, 0, 0))
    darken.set_alpha(255 - int(255 * light_level))
    screen.blit(darken, (0,0))
    
    pygame.display.flip()
    frame_count += 1
    log('DEBUG', f"Frame {frame_count} rendered")

log('INFO', "Game loop ended, quitting Pygame")
pygame.quit()
