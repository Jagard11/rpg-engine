# ./GameV2/world_display.py
# Displays the game world with camera controls, generation UI, escape menu, and dynamic cycles

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator, Tile, assign_biome
from generation_ui import GenerationUI
import sys
import os

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
    sys.exit(1)

# Initialize game state
ui = GenerationUI(SCREEN_WIDTH, SCREEN_HEIGHT)
state = "UI"
camera = None
tiles = None
day_night_gradient = None
seasonal_gradient = None
font = pygame.font.Font(None, 36)
escape_menu_button = pygame.Rect(220, 360, 200, 50)
toggle_day_night_button = pygame.Rect(220, 180, 200, 50)
toggle_seasons_button = pygame.Rect(220, 240, 200, 50)
toggle_terrain_button = pygame.Rect(220, 300, 200, 50)
game_time = 0  # In-game hours
temp_shift = 0
update_counter = 0
day_night_enabled = False  # Debug toggle for gradient visibility
seasons_enabled = False   # Debug toggle for gradient visibility
terrain_enabled = True
map_width = 0
map_height = 0

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
            try:
                seed, map_width, map_height, global_temp_modifier = ui.get_settings()
                info(f"Settings retrieved: width={map_width}, height={map_height}, seed={seed}, temp_mod={global_temp_modifier}")

                info("Generating map...")
                map_gen = MapGenerator(map_width, map_height, seed=seed, global_temp_modifier=global_temp_modifier)
                tiles = map_gen.generate()
                if not tiles:
                    raise ValueError("Map generation returned no tiles")
                info("Map generated successfully")

                info("Initializing camera...")
                camera = Camera(map_width, map_height, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)
                info("Camera initialized")

                day_night_path = f"./GameV2/assets/day_night_{map_width}.png"
                seasonal_path = f"./GameV2/assets/seasonal_{map_height}.png"
                info(f"Loading day-night gradient: {day_night_path}")
                if os.path.exists(day_night_path):
                    day_night_gradient = pygame.image.load(day_night_path).convert_alpha()
                else:
                    error(f"Day-night gradient file not found: {day_night_path}")
                    day_night_gradient = pygame.Surface((map_width, 1), pygame.SRCALPHA)
                    day_night_gradient.fill((128, 128, 128, 255))

                info(f"Loading seasonal gradient: {seasonal_path}")
                if os.path.exists(seasonal_path):
                    seasonal_gradient = pygame.image.load(seasonal_path).convert_alpha()
                else:
                    error(f"Seasonal gradient file not found: {seasonal_path}")
                    seasonal_gradient = pygame.Surface((1, map_height), pygame.SRCALPHA)
                    seasonal_gradient.fill((128, 128, 128, 255))

                temp_shift = global_temp_modifier * 10.0 if global_temp_modifier < 0.1 else global_temp_modifier * 1.2
                state = "WORLD"
                info(f"Switched to WORLD state: {map_width}x{map_height}, seed={seed}, global_temp_modifier={global_temp_modifier}")
            except FileNotFoundError as e:
                error(f"Gradient file error: {e}")
                state = "UI"
                ui.done = False
            except MemoryError as e:
                error(f"Memory error during map generation: {e}")
                state = "UI"
                ui.done = False
            except Exception as e:
                error(f"Unexpected error during map generation: {e}")
                pygame.quit()
                sys.exit(1)

    elif state == "WORLD":
        game_time += 1 / 60  # 1 frame = 1/60 minute
        load_config()

        day_night_pos = int((game_time / 24) % 1.0 * map_width)
        seasonal_pos = int((game_time / 576) % 1.0 * map_height)

        # Update visible tiles every 10 frames (only if gradients enabled for debug)
        update_counter += 1
        if update_counter % 10 == 0 and (day_night_enabled or seasons_enabled):
            cam_tile_x = int(camera.x // TILE_SIZE)
            cam_tile_y = int(camera.y // TILE_SIZE)
            tiles_w = (SCREEN_WIDTH // TILE_SIZE) + 2
            tiles_h = (SCREEN_HEIGHT // TILE_SIZE) + 2
            for y in range(max(0, cam_tile_y - 1), min(map_height, cam_tile_y + tiles_h + 1)):
                for x in range(max(0, cam_tile_x - 1), min(map_width, cam_tile_x + tiles_w + 1)):
                    tile = tiles[y][x]
                    new_temp = tile.base_temp - temp_shift
                    if day_night_enabled:
                        day_x = (x + day_night_pos) % map_width
                        light_factor = day_night_gradient.get_at((day_x, 0))[0] / 255.0
                        new_temp += (light_factor - 0.5) * 0.2
                    if seasons_enabled:
                        season_y = (y + seasonal_pos) % map_height
                        season_factor = (seasonal_gradient.get_at((0, season_y))[0] / 255.0) * 2 - 1
                        new_temp += season_factor * 0.2
                    if abs(new_temp - tile.current_temp) > 0.1:
                        tile.current_temp = new_temp
                        new_biome = assign_biome(tile.continent, tile.elevation, tile.current_moisture, tile.current_temp)
                        if new_biome != tile.biome:
                            tile.biome = new_biome

        # Render terrain with always-on day-night darkening
        camera.render(screen, tiles, debug_seam=show_seam(), terrain_enabled=terrain_enabled,
                      day_night_enabled=day_night_enabled, seasons_enabled=seasons_enabled,
                      day_night_pos=day_night_pos, seasonal_pos=seasonal_pos,
                      day_night_gradient=day_night_gradient, seasonal_gradient=seasonal_gradient)

        keys = pygame.key.get_pressed()
        camera.update(keys, events)

        for event in events:
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "ESCAPE"
                info("Opened escape menu")

    elif state == "ESCAPE":
        screen.fill((50, 50, 50, 128))
        pygame.draw.rect(screen, (0, 200, 0) if day_night_enabled else (100, 100, 100), toggle_day_night_button)
        toggle_day_text = font.render("Toggle Day-Night", True, (255, 255, 255))
        screen.blit(toggle_day_text, (toggle_day_night_button.x + 10, toggle_day_night_button.y + 10))
        pygame.draw.rect(screen, (0, 200, 0) if seasons_enabled else (100, 100, 100), toggle_seasons_button)
        toggle_seasons_text = font.render("Toggle Seasons", True, (255, 255, 255))
        screen.blit(toggle_seasons_text, (toggle_seasons_button.x + 10, toggle_seasons_button.y + 10))
        pygame.draw.rect(screen, (0, 200, 0) if terrain_enabled else (100, 100, 100), toggle_terrain_button)
        toggle_terrain_text = font.render("Toggle Terrain", True, (255, 255, 255))
        screen.blit(toggle_terrain_text, (toggle_terrain_button.x + 10, toggle_terrain_button.y + 10))
        pygame.draw.rect(screen, (0, 200, 0), escape_menu_button)
        button_text = font.render("Return to Generation", True, (255, 255, 255))
        screen.blit(button_text, (escape_menu_button.x + 10, escape_menu_button.y + 10))

        for event in events:
            if event.type == pygame.MOUSEBUTTONDOWN:
                if toggle_day_night_button.collidepoint(event.pos):
                    day_night_enabled = not day_night_enabled
                    info(f"Day-night gradient visibility toggled: {day_night_enabled}")
                elif toggle_seasons_button.collidepoint(event.pos):
                    seasons_enabled = not seasons_enabled
                    info(f"Seasons gradient visibility toggled: {seasons_enabled}")
                elif toggle_terrain_button.collidepoint(event.pos):
                    terrain_enabled = not terrain_enabled
                    info(f"Terrain toggled: {terrain_enabled}")
                elif escape_menu_button.collidepoint(event.pos):
                    state = "UI"
                    ui.done = False
                    info("Returned to generation UI from escape menu")
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "WORLD"
                info("Closed escape menu")

    pygame.display.flip()
    clock.tick(60)

info("Shutting down Pygame")
pygame.quit()