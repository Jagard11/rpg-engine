# ./FrontEnd/CharacterInfo/views/Shared/ClassEditor/__init__.py

from .interface import render_class_editor
from ...JobClasses.database import (
    get_class_details,
    get_class_prerequisites,
    get_class_exclusions,
    save_class
)

__all__ = [
    'render_class_editor',
    'get_class_details',
    'get_class_prerequisites', 
    'get_class_exclusions',
    'save_class'
]

# ./FrontEnd/CharacterInfo/views/Shared/ClassEditor/database.py
"""Database operations for the shared class editor"""

from typing import Dict, List, Optional, Tuple
from ....utils.database import get_db_connection

def get_class_types():
    """Get all class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_types ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_categories(race_only: bool = False):
    """Get class categories based on type"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        if race_only:
            cursor.execute("""
                SELECT id, name 
                FROM class_categories 
                WHERE is_racial = TRUE 
                ORDER BY name
            """)
        else:
            cursor.execute("""
                SELECT id, name 
                FROM class_categories 
                WHERE is_racial = FALSE 
                ORDER BY name
            """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_subcategories():
    """Get all class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_subcategories ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_all_classes(include_racial: bool = False):
    """Get all classes for selection"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                cc.name as category
            FROM classes c
            JOIN class_categories cc ON c.category_id = cc.id
            WHERE c.is_racial = ?
            ORDER BY cc.name, c.name
        """, (include_racial,))
        return [
            {
                'id': row[0],
                'name': row[1],
                'category': row[2]
            }
            for row in cursor.fetchall()
        ]
    finally:
        conn.close()

def get_class_details(class_id: int) -> Optional[Dict]:
    """Get full details of a specific class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT * FROM classes WHERE id = ?
        """, (class_id,))
        result = cursor.fetchone()
        if result:
            columns = [desc[0] for desc in cursor.description]
            return dict(zip(columns, result))
        return None
    finally:
        conn.close()

def get_class_prerequisites(class_id: int) -> List[Dict]:
    """Get prerequisites for a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                prerequisite_group,
                prerequisite_type,
                target_id,
                required_level,
                min_value,
                max_value
            FROM class_prerequisites
            WHERE class_id = ?
            ORDER BY prerequisite_group, id
        """, (class_id,))
        return [
            {
                'prerequisite_group': row[0],
                'prerequisite_type': row[1],
                'target_id': row[2],
                'required_level': row[3],
                'min_value': row[4],
                'max_value': row[5]
            }
            for row in cursor.fetchall()
        ]
    finally:
        conn.close()

def get_class_exclusions(class_id: int) -> List[Dict]:
    """Get exclusions for a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                exclusion_type,
                target_id,
                min_value,
                max_value
            FROM class_exclusions
            WHERE class_id = ?
            ORDER BY id
        """, (class_id,))
        return [
            {
                'exclusion_type': row[0],
                'target_id': row[1],
                'min_value': row[2],
                'max_value': row[3]
            }
            for row in cursor.fetchall()
        ]
    finally:
        conn.close()

def save_class(
    class_data: Dict,
    prerequisites: List[List[Dict]],
    exclusions: List[Dict]
) -> Tuple[bool, str]:
    """Save or update a class with prerequisites and exclusions"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if class_data.get('id'):
            # Update existing class
            cursor.execute("""
                UPDATE classes SET
                    name = ?,
                    description = ?,
                    class_type = ?,
                    is_racial = ?,
                    category_id = ?,
                    subcategory_id = ?,
                    base_hp = ?,
                    base_mp = ?,
                    base_physical_attack = ?,
                    base_physical_defense = ?,
                    base_agility = ?,
                    base_magical_attack = ?,
                    base_magical_defense = ?,
                    base_resistance = ?,
                    base_special = ?,
                    hp_per_level = ?,
                    mp_per_level = ?,
                    physical_attack_per_level = ?,
                    physical_defense_per_level = ?,
                    agility_per_level = ?,
                    magical_attack_per_level = ?,
                    magical_defense_per_level = ?,
                    resistance_per_level = ?,
                    special_per_level = ?
                WHERE id = ?
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['is_racial'],
                class_data['category_id'], class_data['subcategory_id'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'], class_data['hp_per_level'],
                class_data['mp_per_level'], class_data['physical_attack_per_level'],
                class_data['physical_defense_per_level'], class_data['agility_per_level'],
                class_data['magical_attack_per_level'], class_data['magical_defense_per_level'],
                class_data['resistance_per_level'], class_data['special_per_level'],
                class_data['id']
            ))
            class_id = class_data['id']
        else:
            # Create new class
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, is_racial,
                    category_id, subcategory_id,
                    base_hp, base_mp, base_physical_attack, base_physical_defense,
                    base_agility, base_magical_attack, base_magical_defense,
                    base_resistance, base_special,
                    hp_per_level, mp_per_level, physical_attack_per_level,
                    physical_defense_per_level, agility_per_level,
                    magical_attack_per_level, magical_defense_per_level,
                    resistance_per_level, special_per_level
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['is_racial'],
                class_data['category_id'], class_data['subcategory_id'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'], class_data['hp_per_level'],
                class_data['mp_per_level'], class_data['physical_attack_per_level'],
                class_data['physical_defense_per_level'], class_data['agility_per_level'],
                class_data['magical_attack_per_level'], class_data['magical_defense_per_level'],
                class_data['resistance_per_level'], class_data['special_per_level']
            ))
            class_id = cursor.lastrowid

        # Clear existing prerequisites and exclusions
        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", (class_id,))
        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", (class_id,))

        # Add prerequisites
        for group_idx, group in enumerate(prerequisites):
            for prereq in group:
                cursor.execute("""
                    INSERT INTO class_prerequisites (
                        class_id,
                        prerequisite_group,
                        prerequisite_type,
                        target_id,
                        required_level,
                        min_value,
                        max_value
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    class_id,
                    group_idx,
                    prereq['type'],
                    prereq.get('target_id'),
                    prereq.get('required_level'),
                    prereq.get('min_value'),
                    prereq.get('max_value')
                ))
        
        # Add exclusions
        for excl in exclusions:
            cursor.execute("""
                INSERT INTO class_exclusions (
                    class_id,
                    exclusion_type,
                    target_id,
                    min_value,
                    max_value
                ) VALUES (?, ?, ?, ?, ?)
            """, (
                class_id,
                excl['type'],
                excl.get('target_id'),
                excl.get('min_value'),
                excl.get('max_value')
            ))

        cursor.execute("COMMIT")
        return True, "Class saved successfully!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving class: {str(e)}"
    finally:
        conn.close()