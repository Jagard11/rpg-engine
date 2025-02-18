# ./GameV2/camera.py
# Defines a Camera class for panning, zooming, and rendering the game world

import pygame
from logger import info, error

class Camera:
    def __init__(self, map_width, map_height, tile_size, screen_width, screen_height):
        """Initialize the camera with map and screen dimensions."""
        self.map_width = map_width  # In tiles
        self.map_height = map_height  # In tiles
        self.base_tile_size = tile_size  # Base size in pixels
        self.screen_width = screen_width  # Pixels
        self.screen_height = screen_height  # Pixels

        # Camera position (in pixels, relative to world)
        self.x = 0
        self.y = 0  # Will be adjusted by bounds

        # Velocity for momentum (pixels per frame)
        self.vx = 0
        self.vy = 0
        self.acceleration = 0.5  # Speed gain per frame
        self.friction = 0.1  # Speed loss per frame
        self.max_speed = 10  # Cap for momentum

        # Zoom level (1.0 = normal, >1 = zoomed in, <1 = zoomed out)
        self.zoom_level = 1.0
        self.max_zoom = 2.0
        self.zoom_step = 0.1  # Amount per wheel tick
        self.min_zoom = self.screen_height / (self.map_height * self.base_tile_size)
        self.min_zoom = max(0.01, self.min_zoom)

        # Max bounds adjust with zoom
        self.update_bounds()

        # Center the map initially
        self.center_map()

        info(f"Camera initialized: map={map_width}x{map_height}, screen={screen_width}x{screen_height}, min_zoom={self.min_zoom:.3f}")

    def update_bounds(self):
        """Update movement bounds based on current zoom level."""
        self.tile_size = int(self.base_tile_size * self.zoom_level)
        map_pixel_width = self.map_width * self.tile_size
        map_pixel_height = self.map_height * self.tile_size
        self.max_x = max(0, map_pixel_width - self.screen_width)
        self.max_y = max(0, map_pixel_height - self.screen_height)
        info(f"Bounds updated: zoom={self.zoom_level:.3f}, tile_size={self.tile_size}, map_height_px={map_pixel_height}, max_x={self.max_x}, max_y={self.max_y}")

    def center_map(self):
        """Center the map vertically in the screen."""
        map_pixel_height = self.map_height * self.tile_size
        if map_pixel_height < self.screen_height:
            self.y = 0  # No panning needed, offset handled in render
        else:
            self.y = (map_pixel_height - self.screen_height) / 2
            self.y = max(0, min(self.y, self.max_y))

    def zoom(self, amount):
        """Zoom toward the center of the current view, handling wrapping."""
        # Current center point in screen coordinates
        center_screen_x = self.screen_width / 2
        center_screen_y = self.screen_height / 2

        # Current center point in world coordinates (normalized for wrapping)
        map_pixel_width = self.map_width * self.tile_size
        center_world_x = (self.x + center_screen_x) % map_pixel_width
        center_world_y = self.y + center_screen_y

        # Apply zoom
        new_zoom = self.zoom_level + amount
        if self.min_zoom <= new_zoom <= self.max_zoom:
            old_tile_size = self.tile_size
            self.zoom_level = new_zoom
            self.update_bounds()

            # Adjust position to keep the center point stable
            zoom_factor = self.tile_size / old_tile_size
            new_map_pixel_width = self.map_width * self.tile_size
            self.x = (center_world_x * zoom_factor - center_screen_x) % new_map_pixel_width
            self.y = center_world_y * zoom_factor - center_screen_y

            # Clamp to bounds (vertical only, horizontal wraps)
            self.x = self.x % (self.map_width * self.tile_size)  # Ensure wrapping
            self.y = max(0, min(self.y, self.max_y))

    def update(self, keys, events):
        """Update camera position with WASD and momentum."""
        # Acceleration from WASD
        if keys[pygame.K_a]:  # Left
            self.vx -= self.acceleration
        if keys[pygame.K_d]:  # Right
            self.vx += self.acceleration
        if keys[pygame.K_w]:  # Up
            self.vy -= self.acceleration
        if keys[pygame.K_s]:  # Down
            self.vy += self.acceleration

        # Apply friction to slow down
        if abs(self.vx) > 0:
            self.vx -= self.friction * (1 if self.vx > 0 else -1)
            if abs(self.vx) < self.friction:  # Stop if too slow
                self.vx = 0
        if abs(self.vy) > 0:
            self.vy -= self.friction * (1 if self.vy > 0 else -1)
            if abs(self.vy) < self.friction:
                self.vy = 0

        # Cap speed
        self.vx = max(-self.max_speed, min(self.max_speed, self.vx))
        self.vy = max(-self.max_speed, min(self.max_speed, self.vy))

        # Update position
        self.x += self.vx
        self.y += self.vy

        # Wrap horizontally
        self.x = self.x % (self.map_width * self.tile_size)

        # Clamp vertically
        self.y = max(0, min(self.y, self.max_y))

        # Handle mouse wheel zoom
        for event in events:
            if event.type == pygame.MOUSEWHEEL:
                if event.y > 0:  # Wheel up = zoom in
                    self.zoom(self.zoom_step)
                    info(f"Zoom level: {self.zoom_level:.2f}")
                elif event.y < 0:  # Wheel down = zoom out
                    self.zoom(-self.zoom_step)
                    info(f"Zoom level: {self.zoom_level:.2f}")

    def render(self, screen, tiles, debug_seam=False):
        """Render the visible portion of the map, handling wrapping and zoom."""
        screen.fill((0, 0, 0))  # Clear screen to black (shows borders)

        # Calculate vertical offset to center the map
        map_pixel_height = self.map_height * self.tile_size
        if map_pixel_height < self.screen_height:
            vertical_offset = (self.screen_height - map_pixel_height) / 2
        else:
            vertical_offset = 0

        # Convert camera position to tile coordinates
        cam_tile_x = int(self.x // self.tile_size)  # Ensure integer
        cam_tile_y = int(self.y // self.tile_size)  # Ensure integer

        # Number of tiles visible on screen (plus buffer)
        tiles_w = (self.screen_width // self.tile_size) + 2
        tiles_h = (self.screen_height // self.tile_size) + 2

        # Render tiles
        for y in range(tiles_h):
            for x in range(tiles_w):
                # Map tile coordinates with wrapping
                map_x = (cam_tile_x + x) % self.map_width
                map_y = cam_tile_y + y

                # Skip if out of vertical bounds
                if map_y >= self.map_height or map_y < 0:
                    continue

                # Get tile color
                tile_color = tiles[map_y][map_x]

                # Screen position adjusted for camera and centering
                screen_x = (x * self.tile_size) - (self.x % self.tile_size)
                screen_y = (y * self.tile_size) - (self.y % self.tile_size) + vertical_offset

                pygame.draw.rect(
                    screen,
                    tile_color,
                    (screen_x, screen_y, self.tile_size, self.tile_size)
                )

        # Debug seam overlay (yellow lines at left and right edges)
        if debug_seam:
            seam_color = (255, 255, 0)  # Bright yellow
            # Left seam (x=0 in world coordinates)
            seam_x = -(self.x % (self.map_width * self.tile_size))
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)
            # Right seam (x=map_width-1 in world coordinates)
            seam_x = seam_x + (self.map_width * self.tile_size)
            pygame.draw.line(screen, seam_color, (seam_x, 0), (seam_x, self.screen_height), 2)

    def get_position(self):
        """Return current camera position in pixels."""
        return self.x, self.y