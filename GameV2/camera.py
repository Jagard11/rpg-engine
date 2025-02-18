# ./GameV2/camera.py
# Defines a Camera class for panning, zooming, and rendering the game world

import pygame
from logger import info, error
from biome_types import BIOME_TYPES

class Camera:
    def __init__(self, map_width, map_height, tile_size, screen_width, screen_height):
        self.map_width = map_width
        self.map_height = map_height
        self.base_tile_size = tile_size
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.x = 0
        self.y = 0
        self.vx = 0
        self.vy = 0
        self.acceleration = 0.5
        self.friction = 0.1
        self.max_speed = 10
        self.zoom_level = 1.0
        self.max_zoom = 2.0
        self.zoom_step = 0.1
        self.min_zoom = self.screen_height / (self.map_height * self.base_tile_size)
        self.min_zoom = max(0.01, self.min_zoom)
        self.update_bounds()
        self.center_map()

    def update_bounds(self):
        self.tile_size = int(self.base_tile_size * self.zoom_level)
        self.max_x = max(0, self.map_width * self.tile_size - self.screen_width)
        self.max_y = max(0, self.map_height * self.tile_size - self.screen_height)

    def center_map(self):
        self.x = (self.map_width * self.tile_size - self.screen_width) / 2
        self.y = (self.map_height * self.tile_size - self.screen_height) / 2
        self.x = max(0, min(self.x, self.max_x))
        self.y = max(0, min(self.y, self.max_y))

    def zoom(self, amount):
        center_screen_x = self.screen_width / 2
        center_screen_y = self.screen_height / 2
        center_world_x = self.x + center_screen_x
        center_world_y = self.y + center_screen_y
        new_zoom = self.zoom_level + amount
        if self.min_zoom <= new_zoom <= self.max_zoom:
            old_tile_size = self.tile_size
            self.zoom_level = new_zoom
            self.update_bounds()
            zoom_factor = self.tile_size / old_tile_size
            self.x = center_world_x * zoom_factor - center_screen_x
            self.y = center_world_y * zoom_factor - center_screen_y
            self.x = self.x % (self.map_width * self.tile_size)
            self.y = max(0, min(self.y, self.max_y))

    def update(self, keys, events):
        if keys[pygame.K_a]:
            self.vx -= self.acceleration
        if keys[pygame.K_d]:
            self.vx += self.acceleration
        if keys[pygame.K_w]:
            self.vy -= self.acceleration
        if keys[pygame.K_s]:
            self.vy += self.acceleration

        if abs(self.vx) > 0:
            self.vx -= self.friction * (1 if self.vx > 0 else -1)
            if abs(self.vx) < self.friction:
                self.vx = 0
        if abs(self.vy) > 0:
            self.vy -= self.friction * (1 if self.vy > 0 else -1)
            if abs(self.vy) < self.friction:
                self.vy = 0

        self.vx = max(-self.max_speed, min(self.max_speed, self.vx))
        self.vy = max(-self.max_speed, min(self.max_speed, self.vy))

        self.x += self.vx
        self.y += self.vy
        self.x = self.x % (self.map_width * self.tile_size)
        self.y = max(0, min(self.y, self.max_y))

        for event in events:
            if event.type == pygame.MOUSEWHEEL:
                if event.y > 0:
                    self.zoom(self.zoom_step)
                elif event.y < 0:
                    self.zoom(-self.zoom_step)

    def render(self, screen, tiles, debug_seam=False, terrain_enabled=True, day_night_enabled=False, seasons_enabled=False, day_night_pos=0, seasonal_pos=0, day_night_gradient=None, seasonal_gradient=None):
        screen.fill((0, 0, 0))
        cam_tile_x = int(self.x // self.tile_size)
        cam_tile_y = int(self.y // self.tile_size)
        tiles_w = (self.screen_width // self.tile_size) + 2
        tiles_h = (self.screen_height // self.tile_size) + 2

        for y in range(max(0, cam_tile_y - 1), min(self.map_height, cam_tile_y + tiles_h + 1)):
            for x_offset in range(-tiles_w, tiles_w):
                map_x = (cam_tile_x + x_offset) % self.map_width
                screen_x = (x_offset - (self.x / self.tile_size - cam_tile_x)) * self.tile_size
                screen_y = (y * self.tile_size) - self.y
                if 0 <= screen_x < self.screen_width and 0 <= screen_y < self.screen_height:
                    tile = tiles[y][map_x]
                    if terrain_enabled:
                        biome = tile.biome if tile.biome else "GRASSLAND"
                        tile_color = list(BIOME_TYPES[biome]["color"])
                    else:
                        tile_color = [0, 0, 0]

                    # Always-on day-night darkening
                    if day_night_gradient:
                        day_x = (map_x + day_night_pos) % self.map_width
                        light_value = day_night_gradient.get_at((day_x, 0))[0]  # 0–255
                        day_factor = light_value / 255.0  # 0 (night) to 1 (day)
                        for i in range(3):
                            tile_color[i] = int(tile_color[i] * (0.4 + 0.6 * day_factor))  # 0.4–1.0

                    pygame.draw.rect(screen, tuple(tile_color), (screen_x, screen_y, self.tile_size, self.tile_size))

                    # Debug gradient overlays
                    if day_night_enabled and day_night_gradient:
                        day_x = (map_x + day_night_pos) % self.map_width
                        gradient_color = day_night_gradient.get_at((day_x, 0))
                        pygame.draw.rect(screen, gradient_color, (screen_x, screen_y, self.tile_size, self.tile_size), 1)  # Outline for visibility
                    if seasons_enabled and seasonal_gradient:
                        season_y = (y + seasonal_pos) % self.map_height
                        gradient_color = seasonal_gradient.get_at((0, season_y))
                        pygame.draw.rect(screen, gradient_color, (screen_x, screen_y, self.tile_size, self.tile_size), 1)

        if debug_seam:
            seam_color = (255, 255, 0)
            seam_x = -(self.x % (self.map_width * self.tile_size))
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)
            seam_x = seam_x + (self.map_width * self.tile_size)
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)

    def get_position(self):
        return self.x, self.y