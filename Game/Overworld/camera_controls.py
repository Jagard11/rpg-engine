# ./Game/Overworld/camera_controls.py

import pygame
from dataclasses import dataclass
from custom_logging import log, log_method_call

@dataclass
class CameraState:
    """Stores the current state of the camera/viewport"""
    x: float = 0
    y: float = 0
    scale: float = 1.0
    
    def __post_init__(self):
        self.min_scale = 0.25  # Maximum zoom out
        self.max_scale = 4.0   # Maximum zoom in
        self.move_speed = 300  # Pixels per second at scale 1.0
        self.zoom_speed = 0.1  # Scale change per zoom action

    def __str__(self):
        return f"Camera(x={self.x:.1f}, y={self.y:.1f}, scale={self.scale:.2f})"

class CameraController:
    def __init__(self, initial_state=None):
        self.camera = initial_state if initial_state else CameraState()
        log('INFO', f"Camera initialized: {self.camera}")
    
    def handle_input(self, dt):
        """Process keyboard input and update camera state"""
        keys = pygame.key.get_pressed()
        
        # Track if any movement occurred
        moved = False
        
        # Base movement on current zoom level
        move_amount = self.camera.move_speed * dt
        
        # Movement controls (WASD or Arrow keys)
        if keys[pygame.K_w] or keys[pygame.K_UP]:
            self.move(0, -move_amount)
            moved = True
        if keys[pygame.K_s] or keys[pygame.K_DOWN]:
            self.move(0, move_amount)
            moved = True
        if keys[pygame.K_a] or keys[pygame.K_LEFT]:
            self.move(-move_amount, 0)
            moved = True
        if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
            self.move(move_amount, 0)
            moved = True
            
        # Zoom controls (Q/E or +/-)
        if keys[pygame.K_q] or keys[pygame.K_MINUS]:
            self.zoom_out()
            moved = True
        if keys[pygame.K_e] or keys[pygame.K_PLUS]:
            self.zoom_in()
            moved = True
        
        return moved
    
    def move(self, dx, dy):
        """Move the camera by the given delta"""
        self.camera.x += dx / self.camera.scale
        self.camera.y += dy / self.camera.scale
    
    def zoom_in(self):
        """Increase zoom level"""
        new_scale = min(self.camera.scale + self.camera.zoom_speed, self.camera.max_scale)
        if new_scale != self.camera.scale:
            self.camera.scale = new_scale
    
    def zoom_out(self):
        """Decrease zoom level"""
        new_scale = max(self.camera.scale - self.camera.zoom_speed, self.camera.min_scale)
        if new_scale != self.camera.scale:
            self.camera.scale = new_scale
    
    def get_state(self):
        """Return current camera state"""
        return self.camera