# ./GameV2/world_display.py
# Displays the game world with camera controls, generation UI, escape menu, and dynamic cycles

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator, AlphaMap, Chunk, Tile, update_chunk_tiles, assign_biome, recalculate_biome_dist
from generation_ui import GenerationUI

# Pygame setup
pygame.init()

# Window constants
SCREEN_WIDTH = 640
SCREEN_HEIGHT = 480
TILE_SIZE = 32

# Set up the display
try:
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("World Generation Prototype")
    info("Pygame display initialized successfully")
except Exception as e:
    error(f"Failed to initialize Pygame display: {e}")
    pygame.quit()
    exit()

# Initialize game state
ui = GenerationUI(SCREEN_WIDTH, SCREEN_HEIGHT)
state = "UI"
camera = None
tiles = None
chunks = None
day_night_map = None
seasonal_map = None
font = pygame.font.Font(None, 36)
escape_menu_button = pygame.Rect(220, 240, 200, 50)  # Centered "Return to Generation" button
game_time = 0  # In-game hours
temp_shift = 0  # Placeholder for global_temp_modifier effect

# Main loop
running = True
clock = pygame.time.Clock()

while running:
    events = pygame.event.get()
    for event in events:
        if event.type == pygame.QUIT:
            running = False

    if state == "UI":
        for event in events:
            ui.handle_event(event)
        ui.render(screen)
        
        if ui.is_done():
            seed, map_width, map_height, global_temp_modifier = ui.get_settings()
            map_gen = MapGenerator(map_width, map_height, seed=seed, global_temp_modifier=global_temp_modifier)
            tiles = map_gen.generate()
            chunks = map_gen.chunks  # Get chunk grid
            camera = Camera(map_width, map_height, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)
            day_night_map = AlphaMap(map_width, 24)  # Day-night cycle: 24 hours
            seasonal_map = AlphaMap(map_height, 576)  # Seasonal cycle: 576 hours (1 year)
            temp_shift = global_temp_modifier * 10.0 if global_temp_modifier < 0.1 else global_temp_modifier * 1.2
            state = "WORLD"
            info(f"Switched to WORLD state: {map_width}x{map_height}, seed={seed}, global_temp_modifier={global_temp_modifier}")

    elif state == "WORLD":
        # Update game time (1 real minute = 24 in-game hours)
        game_time += 1 / 60  # 1 frame = 1/60 minute
        load_config()

        # Update alpha maps
        day_night_map.update(game_time)
        if int(game_time) % 48 == 0:  # Seasonal update every 48 hours (monthly, 12/year)
            seasonal_map.update(game_time)

        # Update chunks with day-night and seasonal effects
        for chunk in chunks:
            light_factor = day_night_map.get_value(chunk.x * 16)  # 16 tiles per chunk
            season_factor = seasonal_map.get_value(chunk.y * 16)
            chunk.update_season(season_factor)
            chunk.current_temp = chunk.base_temp + (light_factor - 0.5) * 0.2 + chunk.last_season_factor * 0.2 - temp_shift
            if abs(chunk.current_temp - chunk.last_temp) > 0.1:  # Threshold for update
                update_chunk_tiles(chunk)
                chunk.last_temp = chunk.current_temp

        # Camera controls
        keys = pygame.key.get_pressed()
        camera.update(keys, events)
        camera.render(screen, tiles, debug_seam=show_seam())

        # Escape menu trigger
        for event in events:
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "ESCAPE"
                info("Opened escape menu")

    elif state == "ESCAPE":
        screen.fill((50, 50, 50, 128))  # Semi-transparent overlay
        pygame.draw.rect(screen, (0, 200, 0), escape_menu_button)
        button_text = font.render("Return to Generation", True, (255, 255, 255))
        screen.blit(button_text, (escape_menu_button.x + 10, escape_menu_button.y + 10))

        for event in events:
            if event.type == pygame.MOUSEBUTTONDOWN and escape_menu_button.collidepoint(event.pos):
                state = "UI"
                ui.done = False
                info("Returned to generation UI from escape menu")
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "WORLD"
                info("Closed escape menu")

    pygame.display.flip()
    clock.tick(60)  # Cap at 60 FPS

# Cleanup
info("Shutting down Pygame")
pygame.quit()