# ./GameV2/camera.py
# Defines a Camera class for panning, zooming, and rendering the game world

import pygame
from logger import info, error
from biome_types import BIOME_TYPES  # Import for color lookup

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
        info(f"Camera initialized: map={map_width}x{map_height}, screen={screen_width}x{screen_height}, min_zoom={self.min_zoom:.3f}")

    def update_bounds(self):
        self.tile_size = int(self.base_tile_size * self.zoom_level)
        map_pixel_width = self.map_width * self.tile_size
        map_pixel_height = self.map_height * self.tile_size
        self.max_x = max(0, map_pixel_width - self.screen_width)
        self.max_y = max(0, map_pixel_height - self.screen_height)
        info(f"Bounds updated: zoom={self.zoom_level:.3f}, tile_size={self.tile_size}, map_height_px={map_pixel_height}, max_x={self.max_x}, max_y={self.max_y}")

    def center_map(self):
        map_pixel_height = self.map_height * self.tile_size
        if map_pixel_height < self.screen_height:
            self.y = 0
        else:
            self.y = (map_pixel_height - self.screen_height) / 2
            self.y = max(0, min(self.y, self.max_y))

    def zoom(self, amount):
        center_screen_x = self.screen_width / 2
        center_screen_y = self.screen_height / 2
        center_world_x = (self.x + center_screen_x) % (self.map_width * self.tile_size)
        center_world_y = self.y + center_screen_y
        new_zoom = self.zoom_level + amount
        if self.min_zoom <= new_zoom <= self.max_zoom:
            old_tile_size = self.tile_size
            self.zoom_level = new_zoom
            self.update_bounds()
            zoom_factor = self.tile_size / old_tile_size
            new_map_pixel_width = self.map_width * self.tile_size
            self.x = (center_world_x * zoom_factor - center_screen_x) % new_map_pixel_width
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
                    info(f"Zoom level: {self.zoom_level:.2f}")
                elif event.y < 0:
                    self.zoom(-self.zoom_step)
                    info(f"Zoom level: {self.zoom_level:.2f}")

    def render(self, screen, tiles, debug_seam=False):
        screen.fill((0, 0, 0))
        map_pixel_height = self.map_height * self.tile_size
        vertical_offset = (self.screen_height - map_pixel_height) / 2 if map_pixel_height < self.screen_height else 0
        cam_tile_x = int(self.x // self.tile_size)
        cam_tile_y = int(self.y // self.tile_size)
        tiles_w = (self.screen_width // self.tile_size) + 2
        tiles_h = (self.screen_height // self.tile_size) + 2

        for y in range(tiles_h):
            for x in range(tiles_w):
                map_x = (cam_tile_x + x) % self.map_width
                map_y = cam_tile_y + y
                if map_y >= self.map_height or map_y < 0:
                    continue
                biome = tiles[map_y][map_x]  # Get biome key
                tile_color = BIOME_TYPES[biome]["color"]  # Lookup color
                screen_x = (x * self.tile_size) - (self.x % self.tile_size)
                screen_y = (y * self.tile_size) - (self.y % self.tile_size) + vertical_offset
                pygame.draw.rect(screen, tile_color, (screen_x, screen_y, self.tile_size, self.tile_size))

        if debug_seam:
            seam_color = (255, 255, 0)
            seam_x = -(self.x % (self.map_width * self.tile_size))
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)
            seam_x = seam_x + (self.map_width * self.tile_size)
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)

    def get_position(self):
        return self.x, self.y