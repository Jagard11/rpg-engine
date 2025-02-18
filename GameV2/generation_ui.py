# ./GameV2/generation_ui.py
# Handles the generation UI screen with seed, dimensions, and generate button

import pygame
from logger import info, error

class GenerationUI:
    def __init__(self, screen_width, screen_height):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.font = pygame.font.Font(None, 36)
        self.small_font = pygame.font.Font(None, 24)
        self.active_field = None  # Tracks which text field is being edited
        self.fields = {
            "seed": {"value": "42", "rect": pygame.Rect(300, 100, 200, 40), "label": "Seed:"},
            "width": {"value": "400", "rect": pygame.Rect(300, 150, 200, 40), "label": "Map Width:"},
            "height": {"value": "200", "rect": pygame.Rect(300, 200, 200, 40), "label": "Map Height:"}
        }
        self.generate_button = pygame.Rect(300, 300, 200, 50)
        self.done = False  # Signals when to proceed to world generation
        info("GenerationUI initialized")

    def handle_event(self, event):
        """Process events for text input and button clicks."""
        if event.type == pygame.MOUSEBUTTONDOWN:
            mouse_pos = event.pos
            # Check if clicking a text field
            for field in self.fields:
                if self.fields[field]["rect"].collidepoint(mouse_pos):
                    self.active_field = field
                    return
            self.active_field = None
            # Check if clicking the generate button
            if self.generate_button.collidepoint(mouse_pos):
                self.done = True
                info("Generate button clicked")

        elif event.type == pygame.KEYDOWN and self.active_field:
            current_value = self.fields[self.active_field]["value"]
            if event.key == pygame.K_BACKSPACE:
                self.fields[self.active_field]["value"] = current_value[:-1]
            elif event.key == pygame.K_RETURN:
                self.active_field = None
            elif event.unicode.isdigit() and len(current_value) < 10:
                self.fields[self.active_field]["value"] += event.unicode

    def render(self, screen):
        """Draw the UI elements on the screen."""
        screen.fill((50, 50, 50))  # Dark gray background

        # Draw labels and text fields
        for field, data in self.fields.items():
            label_surface = self.font.render(data["label"], True, (255, 255, 255))
            screen.blit(label_surface, (100, data["rect"].y + 5))
            pygame.draw.rect(screen, (255, 255, 255) if field == self.active_field else (200, 200, 200),
                            data["rect"], 2 if field != self.active_field else 4)
            text_surface = self.font.render(data["value"], True, (255, 255, 255))
            screen.blit(text_surface, (data["rect"].x + 5, data["rect"].y + 5))

        # Draw generate button
        pygame.draw.rect(screen, (0, 200, 0), self.generate_button)
        button_text = self.font.render("Generate Map", True, (255, 255, 255))
        screen.blit(button_text, (self.generate_button.x + 10, self.generate_button.y + 10))

    def get_settings(self):
        """Return the current settings as a tuple (seed, width, height)."""
        try:
            seed = int(self.fields["seed"]["value"])
            width = int(self.fields["width"]["value"])
            height = int(self.fields["height"]["value"])
            if width <= 0 or height <= 0:
                raise ValueError("Width and height must be positive")
            return seed, width, height
        except ValueError as e:
            error(f"Invalid input: {e}. Using defaults (42, 400, 200)")
            return 42, 400, 200

    def is_done(self):
        """Check if the user has clicked the generate button."""
        return self.done