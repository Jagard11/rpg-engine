# ./GameV2/world_display.py
# Displays the game world with camera controls and debug features, now with generation UI

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator
from generation_ui import GenerationUI  # New import

# Pygame setup
pygame.init()

# Window constants (independent of map size)
SCREEN_WIDTH = 640  # Pixels
SCREEN_HEIGHT = 480  # Pixels

# Default map constants (overridden by UI)
TILE_SIZE = 32  # Base size of each tile in pixels

# Set up the display
try:
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("World Generation Prototype")
    info("Pygame display initialized successfully")
except Exception as e:
    error(f"Failed to initialize Pygame display: {e}")
    pygame.quit()
    exit()

# Initialize UI
ui = GenerationUI(SCREEN_WIDTH, SCREEN_HEIGHT)
state = "UI"  # Start in UI mode
camera = None
tiles = None

# Main loop
running = True
clock = pygame.time.Clock()

while running:
    events = pygame.event.get()
    for event in events:
        if event.type == pygame.QUIT:
            running = False

    if state == "UI":
        # Handle UI events and rendering
        for event in events:
            ui.handle_event(event)
        ui.render(screen)
        
        if ui.is_done():
            # Generate map with user settings
            seed, map_width, map_height = ui.get_settings()
            map_gen = MapGenerator(map_width, map_height, seed=seed)
            tiles = map_gen.generate()
            camera = Camera(map_width, map_height, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)
            state = "WORLD"
            info(f"Switched to WORLD state with seed={seed}, map={map_width}x{map_height}")

    elif state == "WORLD":
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