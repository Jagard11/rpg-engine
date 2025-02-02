# ./Game/main.py

import pygame
import sqlite3
import requests
from math import cos, sin, radians

# Initialize pygame
pygame.init()

# Constants
WIDTH, HEIGHT = 800, 600
FOV_ANGLE = 60  # Degrees
FOV_LENGTH = 200
BACKGROUND_COLOR = (30, 30, 30)
NORMAL_SPEED = 5
SPRINT_SPEED = 10
STRAFE_SPEED = 5
ROTATION_SPEED = 5  # Degrees per key press

# Initialize screen
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("2D Game Engine Prototype")

# Database setup
conn = sqlite3.connect("game_data.db")
cursor = conn.cursor()
cursor.execute("""
CREATE TABLE IF NOT EXISTS game_objects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    x INTEGER,
    y INTEGER,
    type TEXT
)
""")
conn.commit()

def send_to_oobabooga(data):
    url = "http://your-oobabooga-server/api"
    response = requests.post(url, json=data)
    return response.json()

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.angle = 0  # Facing angle in degrees

    def move_forward_backward(self, speed):
        rad_angle = radians(self.angle)
        self.x += cos(rad_angle) * speed
        self.y += sin(rad_angle) * speed

    def strafe_left_right(self, speed):
        rad_angle = radians(self.angle + 90)  # Perpendicular to facing direction
        self.x += cos(rad_angle) * speed
        self.y += sin(rad_angle) * speed

    def rotate(self, da):
        self.angle = (self.angle + da) % 360

    def get_fov_cone(self):
        cone_points = []
        for i in range(-FOV_ANGLE//2, FOV_ANGLE//2, 5):
            rad_angle = radians(self.angle + i)
            end_x = self.x + cos(rad_angle) * FOV_LENGTH
            end_y = self.y + sin(rad_angle) * FOV_LENGTH
            cone_points.append((end_x, end_y))
        return cone_points

# Game Loop
player = Player(WIDTH // 2, HEIGHT // 2)
running = True
clock = pygame.time.Clock()

while running:
    screen.fill(BACKGROUND_COLOR)
    
    # Event handling
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
    
    keys = pygame.key.get_pressed()
    forward_speed = SPRINT_SPEED if keys[pygame.K_LSHIFT] or keys[pygame.K_RSHIFT] else NORMAL_SPEED
    
    if keys[pygame.K_w]:
        player.move_forward_backward(forward_speed)
    if keys[pygame.K_s]:
        player.move_forward_backward(-NORMAL_SPEED)
    if keys[pygame.K_a]:
        player.strafe_left_right(-STRAFE_SPEED)
    if keys[pygame.K_d]:
        player.strafe_left_right(STRAFE_SPEED)
    if keys[pygame.K_q]:
        player.rotate(-ROTATION_SPEED)
    if keys[pygame.K_e]:
        player.rotate(ROTATION_SPEED)
    
    # Render FOV Cone
    fov_cone = player.get_fov_cone()
    pygame.draw.polygon(screen, (255, 255, 0, 100), [(player.x, player.y)] + fov_cone, 1)
    
    pygame.display.flip()
    clock.tick(60)

pygame.quit()
conn.close()
