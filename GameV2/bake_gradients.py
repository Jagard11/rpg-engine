# ./GameV2/bake_gradients.py
# Generates day-night and seasonal gradient maps as PNGs for each map size

import pygame
import os

pygame.init()

# Map sizes
sizes = [
    {"name": "tiny", "width": 64, "height": 32},
    {"name": "small", "width": 128, "height": 64},
    {"name": "medium", "width": 256, "height": 128},
    {"name": "large", "width": 512, "height": 256},
    {"name": "huge", "width": 1024, "height": 512}
]

# Output directory
output_dir = "./GameV2/assets/"
os.makedirs(output_dir, exist_ok=True)

for size in sizes:
    width = size["width"]
    height = size["height"]
    
    # Day-night gradient (horizontal, 0–255)
    day_night_surface = pygame.Surface((width, 1), depth=24)  # 24-bit RGB, no alpha
    for x in range(width):
        # Sinusoidal gradient for smooth day-night transition
        value = int(127.5 * (1 + pygame.math.Vector2(1, 0).rotate(360 * x / width).x))
        day_night_surface.set_at((x, 0), (value, value, value))
    pygame.image.save(day_night_surface, f"{output_dir}/day_night_{width}.png")
    print(f"Generated day_night_{width}.png")

    # Seasonal gradient (vertical, 0–255)
    seasonal_surface = pygame.Surface((1, height), depth=24)  # 24-bit RGB, no alpha
    for y in range(height):
        # Linear gradient from top (0) to bottom (255)
        value = int(255 * y / (height - 1))
        seasonal_surface.set_at((0, y), (value, value, value))
    pygame.image.save(seasonal_surface, f"{output_dir}/seasonal_{height}.png")
    print(f"Generated seasonal_{height}.png")

pygame.quit()