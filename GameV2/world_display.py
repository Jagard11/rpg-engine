# ./GameV2/world_display.py
# Displays the game world with camera controls, generation UI, and escape menu

import pygame
from biome_types import BIOME_TYPES
from logger import info, error, load_config, show_seam
from camera import Camera
from map_generator import MapGenerator
from generation_ui import GenerationUI

pygame.init()

SCREEN_WIDTH = 640
SCREEN_HEIGHT = 480
TILE_SIZE = 32

try:
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("World Generation Prototype")
    info("Pygame display initialized successfully")
except Exception as e:
    error(f"Failed to initialize Pygame display: {e}")
    pygame.quit()
    exit()

ui = GenerationUI(SCREEN_WIDTH, SCREEN_HEIGHT)
state = "UI"
camera = None
tiles = None
font = pygame.font.Font(None, 36)
escape_menu_button = pygame.Rect(220, 240, 200, 50)  # Centered "Return to Generation" button

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
        
        # In the "UI" state block:
        if ui.is_done():
            seed, map_width, map_height, global_temp_modifier = ui.get_settings()
            map_gen = MapGenerator(map_width, map_height, seed=seed, global_temp_modifier=global_temp_modifier)
            tiles = map_gen.generate()
            camera = Camera(map_width, map_height, TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT)
            state = "WORLD"
            info(f"Switched to WORLD state with seed={seed}, map={map_width}x{map_height}, global_temp_modifier={global_temp_modifier}")

    elif state == "WORLD":
        load_config()
        keys = pygame.key.get_pressed()
        camera.update(keys, events)
        camera.render(screen, tiles, debug_seam=show_seam())

        # Check for Escape key to open menu
        for event in events:
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "ESCAPE"
                info("Opened escape menu")

    elif state == "ESCAPE":
        # Render escape menu
        screen.fill((50, 50, 50, 128))  # Semi-transparent overlay
        pygame.draw.rect(screen, (0, 200, 0), escape_menu_button)
        button_text = font.render("Return to Generation", True, (255, 255, 255))
        screen.blit(button_text, (escape_menu_button.x + 10, escape_menu_button.y + 10))

        for event in events:
            if event.type == pygame.MOUSEBUTTONDOWN and escape_menu_button.collidepoint(event.pos):
                state = "UI"
                ui.done = False  # Reset UI state
                info("Returned to generation UI from escape menu")
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                state = "WORLD"  # Close menu with Escape
                info("Closed escape menu")

    pygame.display.flip()
    clock.tick(60)

info("Shutting down Pygame")
pygame.quit()