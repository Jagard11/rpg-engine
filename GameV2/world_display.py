# ./GameV2/world_display.py
# Displays the game world with camera controls, generation UI, escape menu, and dynamic cycles

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator, Tile, assign_biome
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
day_night_gradient = None
seasonal_gradient = None
font = pygame.font.Font(None, 36)
escape_menu_button = pygame.Rect(220, 240, 200, 50)
game_time = 0  # In-game hours
temp_shift = 0

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
            camera = Camera(map_width, map_height, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)
            # Load pre-baked gradient maps
            day_night_gradient = pygame.image.load(f"./GameV2/assets/day_night_{map_width}.png").convert()
            seasonal_gradient = pygame.image.load(f"./GameV2/assets/seasonal_{map_height}.png").convert()
            temp_shift = global_temp_modifier * 10.0 if global_temp_modifier < 0.1 else global_temp_modifier * 1.2
            state = "WORLD"
            info(f"Switched to WORLD state: {map_width}x{map_height}, seed={seed}, global_temp_modifier={global_temp_modifier}")

    elif state == "WORLD":
        # Update game time (1 real minute = 24 in-game hours)
        game_time += 1 / 60  # 1 frame = 1/60 minute
        load_config()

        # Update dynamic effects per tile using gradient maps
        day_night_pos = int((game_time / 24) % 1.0 * map_width)  # Scroll day-night
        seasonal_pos = int((game_time / 576) % 1.0 * map_height)  # Shift seasonal
        for y in range(map_height):
            for x in range(map_width):
                tile = tiles[y][x]
                day_x = (x + day_night_pos) % map_width
                light_factor = day_night_gradient.get_at((day_x, 0))[0] / 255.0  # 0–1
                season_factor = (seasonal_gradient.get_at((0, (y + seasonal_pos) % map_height))[0] / 255.0) * 2 - 1  # 0–255 to -1–1
                tile.current_temp = tile.base_temp + (light_factor - 0.5) * 0.2 + season_factor * 0.2 - temp_shift
                new_biome = assign_biome(tile.continent, tile.elevation, tile.current_moisture, tile.current_temp)  # Fixed typo here
                if new_biome != tile.biome:
                    tile.biome = new_biome

        # Camera controls and rendering
        keys = pygame.key.get_pressed()
        camera.update(keys, events)
        camera.render(screen, tiles, debug_seam=show_seam())

        for event in events:
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "ESCAPE"
                info("Opened escape menu")

    elif state == "ESCAPE":
        screen.fill((50, 50, 50, 128))
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
    clock.tick(60)

info("Shutting down Pygame")
pygame.quit()