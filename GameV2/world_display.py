# ./GameV2/world_display.py
# Displays the game world with camera controls and debug features

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator

# Pygame setup
pygame.init()

# Window constants (independent of map size)
SCREEN_WIDTH = 640  # Pixels
SCREEN_HEIGHT = 480  # Pixels

# Map constants (independent of screen)
TILE_SIZE = 32  # Base size of each tile in pixels
MAP_WIDTH = 400  # Number of tiles wide
MAP_HEIGHT = 200  # Number of tiles tall

# Set up the display
try:
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("World Generation Prototype")
    info("Pygame display initialized successfully")
except Exception as e:
    error(f"Failed to initialize Pygame display: {e}")
    pygame.quit()
    exit()

# Generate the world
map_gen = MapGenerator(MAP_WIDTH, MAP_HEIGHT, seed=42)
tiles = map_gen.generate()

# Initialize camera
camera = Camera(MAP_WIDTH, MAP_HEIGHT, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)

# Main loop
running = True
clock = pygame.time.Clock()

while running:
    events = pygame.event.get()
    for event in events:
        if event.type == pygame.QUIT:
            running = False

    # Reload config each frame to reflect changes
    load_config()

    # Update camera based on input
    keys = pygame.key.get_pressed()
    camera.update(keys, events)

    # Render the world through the camera
    camera.render(screen, tiles, debug_seam=show_seam())

    # Update display
    pygame.display.flip()
    clock.tick(60)  # Cap at 60 FPS

# Cleanup
info("Shutting down Pygame")
pygame.quit()