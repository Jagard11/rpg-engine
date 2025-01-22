# CharacterManagement/CharacterEditor/models/Character.py

from dataclasses import dataclass
from typing import Optional

@dataclass
class Character:
    """Core character data model"""
    id: int
    first_name: str
    middle_name: Optional[str]
    last_name: Optional[str]
    bio: Optional[str]
    total_level: int
    birth_place: Optional[str]
    age: Optional[int]
    karma: int
    talent: Optional[str]
    race_category_id: int
    is_active: bool
    created_at: str
    updated_at: str