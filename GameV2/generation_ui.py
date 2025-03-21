# ./GameV2/generation_ui.py
import pygame
from logger import info, error

class GenerationUI:
    def __init__(self, screen_width, screen_height):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.font = pygame.font.Font(None, 36)
        self.small_font = pygame.font.Font(None, 24)
        self.active_field = None
        self.size_options = [
            {"name": "Tiny", "width": 64, "height": 32},
            {"name": "Small", "width": 128, "height": 64},
            {"name": "Medium", "width": 256, "height": 128},
            {"name": "Large", "width": 512, "height": 256},
            {"name": "Huge", "width": 1024, "height": 512}
        ]
        self.selected_size = 2
        self.fields = {
            "seed": {"value": "42", "rect": pygame.Rect(300, 100, 200, 40), "label": "Seed:"},
            "global_temp_modifier": {"value": "0.1", "rect": pygame.Rect(300, 150, 200, 40), "label": "Global Temp (0.001-1.0):"}
        }
        self.size_rect = pygame.Rect(300, 200, 200, 40)
        self.generate_button = pygame.Rect(300, 300, 200, 50)
        self.done = False
        info("GenerationUI initialized")

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            mouse_pos = event.pos
            for field in self.fields:
                if self.fields[field]["rect"].collidepoint(mouse_pos):
                    self.active_field = field
                    return
            if self.size_rect.collidepoint(mouse_pos):
                self.selected_size = (self.selected_size + 1) % len(self.size_options)
                info(f"Map size selected: {self.size_options[self.selected_size]['name']}")
                return
            self.active_field = None
            if self.generate_button.collidepoint(mouse_pos):
                self.done = True
                info("Generate button clicked")
        elif event.type == pygame.KEYDOWN and self.active_field:
            current_value = self.fields[self.active_field]["value"]
            if event.key == pygame.K_BACKSPACE:
                self.fields[self.active_field]["value"] = current_value[:-1]
            elif event.key == pygame.K_RETURN:
                self.active_field = None
            elif event.unicode.isdigit() or (event.unicode == '.' and '.' not in current_value):
                if len(current_value) < 10:
                    self.fields[self.active_field]["value"] += event.unicode

    def render(self, screen):
        screen.fill((50, 50, 50))
        for field, data in self.fields.items():
            label_surface = self.font.render(data["label"], True, (255, 255, 255))
            label_width = label_surface.get_width()
            label_x = 290 - label_width
            screen.blit(label_surface, (label_x, data["rect"].y + 5))
            pygame.draw.rect(screen, (255, 255, 255) if field == self.active_field else (200, 200, 200),
                             data["rect"], 2 if field != self.active_field else 4)
            text_surface = self.font.render(data["value"], True, (255, 255, 255))
            screen.blit(text_surface, (data["rect"].x + 5, data["rect"].y + 5))
        size_label = self.font.render("Map Size:", True, (255, 255, 255))
        label_width = size_label.get_width()
        screen.blit(size_label, (290 - label_width, self.size_rect.y + 5))
        pygame.draw.rect(screen, (200, 200, 200), self.size_rect, 2)
        size_text = self.font.render(self.size_options[self.selected_size]["name"], True, (255, 255, 255))
        screen.blit(size_text, (self.size_rect.x + 5, self.size_rect.y + 5))
        pygame.draw.rect(screen, (0, 200, 0), self.generate_button)
        button_text = self.font.render("Generate Map", True, (255, 255, 255))
        screen.blit(button_text, (self.generate_button.x + 10, self.generate_button.y + 10))

    def get_settings(self):
        try:
            seed = int(self.fields["seed"]["value"])
            width = self.size_options[self.selected_size]["width"]
            height = self.size_options[self.selected_size]["height"]
            global_temp_modifier = float(self.fields["global_temp_modifier"]["value"])
            if not 0.001 <= global_temp_modifier <= 1.0:
                raise ValueError("Global temp modifier must be between 0.001 and 1.0")
            return seed, width, height, global_temp_modifier
        except ValueError as e:
            error(f"Invalid input: {e}. Using defaults (42, 256, 128, 0.1)")
            return 42, 256, 128, 0.1

    def is_done(self):
        return self.done