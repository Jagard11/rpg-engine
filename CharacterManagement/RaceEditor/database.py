# ./CharacterManagement/RaceEditor/database.py

import sqlite3
from typing import List, Dict, Tuple, Optional

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def check_name_exists(name: str, exclude_id: Optional[int] = None) -> bool:
    """Check if a class name already exists
    
    Args:
        name: The name to check
        exclude_id: Optional ID to exclude from the check (for updates)
        
    Returns:
        bool: True if name exists, False otherwise
    """
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        if exclude_id is not None:
            cursor.execute("""
                SELECT COUNT(*) FROM classes 
                WHERE name = ? AND id != ?
            """, (name, exclude_id))
        else:
            cursor.execute("""
                SELECT COUNT(*) FROM classes 
                WHERE name = ?
            """, (name,))
        return cursor.fetchone()[0] > 0
    finally:
        conn.close()

def save_race(race_data: Dict) -> Tuple[bool, str]:
    """Save or update a racial class"""
    # First check if name is already taken
    if check_name_exists(race_data['name'], race_data.get('id')):
        return False, f"Error saving race: A class with the name '{race_data['name']}' already exists"

    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if race_data.get('id'):
            # Update existing race
            cursor.execute("""
                UPDATE classes 
                SET name = ?,
                    description = ?,
                    class_type = ?,
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
                WHERE id = ? AND is_racial = TRUE
            """, (
                race_data['name'], race_data['description'],
                race_data['class_type'], race_data['category_id'],
                race_data['subcategory_id'],
                race_data['base_hp'], race_data['base_mp'],
                race_data['base_physical_attack'], race_data['base_physical_defense'],
                race_data['base_agility'], race_data['base_magical_attack'],
                race_data['base_magical_defense'], race_data['base_resistance'],
                race_data['base_special'],
                race_data['hp_per_level'], race_data['mp_per_level'],
                race_data['physical_attack_per_level'], race_data['physical_defense_per_level'],
                race_data['agility_per_level'], race_data['magical_attack_per_level'],
                race_data['magical_defense_per_level'], race_data['resistance_per_level'],
                race_data['special_per_level'],
                race_data['id']
            ))
        else:
            # Insert new race
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, category_id,
                    subcategory_id, is_racial,
                    base_hp, base_mp,
                    base_physical_attack, base_physical_defense,
                    base_agility, base_magical_attack,
                    base_magical_defense, base_resistance,
                    base_special,
                    hp_per_level, mp_per_level,
                    physical_attack_per_level, physical_defense_per_level,
                    agility_per_level, magical_attack_per_level,
                    magical_defense_per_level, resistance_per_level,
                    special_per_level
                ) VALUES (?, ?, ?, ?, ?, TRUE, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                race_data['name'], race_data['description'],
                race_data['class_type'], race_data['category_id'],
                race_data['subcategory_id'],
                race_data['base_hp'], race_data['base_mp'],
                race_data['base_physical_attack'], race_data['base_physical_defense'],
                race_data['base_agility'], race_data['base_magical_attack'],
                race_data['base_magical_defense'], race_data['base_resistance'],
                race_data['base_special'],
                race_data['hp_per_level'], race_data['mp_per_level'],
                race_data['physical_attack_per_level'], race_data['physical_defense_per_level'],
                race_data['agility_per_level'], race_data['magical_attack_per_level'],
                race_data['magical_defense_per_level'], race_data['resistance_per_level'],
                race_data['special_per_level']
            ))

        cursor.execute("COMMIT")
        return True, f"Race {'updated' if race_data.get('id') else 'created'} successfully!"
    
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving race: {str(e)}"
    finally:
        conn.close()

def delete_race(race_id: int) -> Tuple[bool, str]:
    """Delete a racial class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Check if race exists and is racial
        cursor.execute("""
            SELECT COUNT(*) FROM classes 
            WHERE id = ? AND is_racial = TRUE
        """, (race_id,))
        if cursor.fetchone()[0] == 0:
            return False, "Race not found"

        # Check if race is in use
        cursor.execute("""
            SELECT COUNT(*) FROM character_class_progression
            WHERE class_id = ?
        """, (race_id,))
        if cursor.fetchone()[0] > 0:
            return False, "Cannot delete: Race is currently used by one or more characters"

        cursor.execute("BEGIN TRANSACTION")
        
        # Delete race-specific data (if any)
        
        # Delete the race
        cursor.execute("DELETE FROM classes WHERE id = ? AND is_racial = TRUE", (race_id,))

        cursor.execute("COMMIT")
        return True, "Race deleted successfully"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error deleting race: {str(e)}"
    finally:
        conn.close()

def get_race_details(race_id: int) -> Optional[Dict]:
    """Get full details of a specific racial class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.*,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE c.id = ? AND c.is_racial = TRUE
        """, (race_id,))
        
        result = cursor.fetchone()
        if result:
            columns = [desc[0] for desc in cursor.description]
            return dict(zip(columns, result))
        return None
    finally:
        conn.close()

def get_all_races() -> List[Dict]:
    """Get list of all racial classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name,
                c.base_hp,
                c.base_mp,
                c.base_physical_attack,
                c.base_physical_defense,
                c.base_agility,
                c.base_magical_attack,
                c.base_magical_defense,
                c.base_resistance,
                c.base_special,
                c.hp_per_level,
                c.mp_per_level,
                c.physical_attack_per_level,
                c.physical_defense_per_level,
                c.agility_per_level,
                c.magical_attack_per_level,
                c.magical_defense_per_level,
                c.resistance_per_level,
                c.special_per_level
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE c.is_racial = TRUE
            ORDER BY cc.name, c.name
        """)
        
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()