# ./FrontEnd/CharacterInfo/views/JobClasses/database.py

from typing import Dict, List, Optional, Tuple
from FrontEnd.CharacterInfo.utils.database import get_db_connection

def get_class_types() -> List[Dict]:
    """Get all class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_types ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_categories() -> List[Dict]:
    """Get non-racial class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = 0 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_subcategories() -> List[Dict]:
    """Get all class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_subcategories ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_all_classes() -> List[Dict]:
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
            ORDER BY c.is_racial, cc.name, c.name
        """)
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
            SELECT * FROM classes WHERE id = ? AND is_racial = 0
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

def save_class(class_data: Dict) -> Tuple[bool, str]:
    """Save or update a class record"""
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
                WHERE id = ? AND is_racial = 0
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['category_id'],
                class_data['subcategory_id'],
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
                ) VALUES (?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['category_id'],
                class_data['subcategory_id'],
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
        
        cursor.execute("COMMIT")
        return True, "Class saved successfully!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving class: {str(e)}"
    finally:
        conn.close()

def delete_class(class_id: int) -> Tuple[bool, str]:
    """Delete a job class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Check if class is in use
        cursor.execute("""
            SELECT COUNT(*) 
            FROM character_class_progression 
            WHERE class_id = ?
        """, (class_id,))
        if cursor.fetchone()[0] > 0:
            return False, "Cannot delete: Class is currently in use by one or more characters"

        # Check if it's a racial class
        cursor.execute("SELECT is_racial FROM classes WHERE id = ?", (class_id,))
        result = cursor.fetchone()
        if not result or result[0]:
            return False, "Cannot delete: Invalid class ID or racial class"

        cursor.execute("BEGIN TRANSACTION")
        
        # Delete prerequisites
        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", (class_id,))
        
        # Delete exclusions
        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", (class_id,))
        
        # Delete class
        cursor.execute("DELETE FROM classes WHERE id = ? AND is_racial = 0", (class_id,))
        
        cursor.execute("COMMIT")
        return True, "Class deleted successfully"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error deleting class: {str(e)}"
    finally:
        conn.close()

def copy_class(class_id: int) -> Tuple[bool, str, Optional[int]]:
    """Copy an existing class and its related data"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Start transaction
        cursor.execute("BEGIN TRANSACTION")
        
        # Get original class data
        cursor.execute("""
            SELECT * FROM classes WHERE id = ? AND is_racial = 0
        """, (class_id,))
        original_class = cursor.fetchone()
        if not original_class:
            return False, "Class not found or is a racial class", None
            
        columns = [desc[0] for desc in cursor.description]
        class_data = dict(zip(columns, original_class))
        
        # Create new name for copied class
        new_name = f"{class_data['name']} (Copy)"
        base_name = new_name
        counter = 1
        while True:
            cursor.execute("SELECT COUNT(*) FROM classes WHERE name = ?", (new_name,))
            if cursor.fetchone()[0] == 0:
                break
            counter += 1
            new_name = f"{base_name} {counter}"
        
        # Insert new class
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
            ) VALUES (?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            new_name, class_data['description'],
            class_data['class_type'], class_data['category_id'],
            class_data['subcategory_id'],
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
        
        new_class_id = cursor.lastrowid
        
        # Copy prerequisites
        cursor.execute("""
            INSERT INTO class_prerequisites (
                class_id, prerequisite_group, prerequisite_type,
                target_id, required_level, min_value, max_value
            )
            SELECT 
                ?, prerequisite_group, prerequisite_type,
                target_id, required_level, min_value, max_value
            FROM class_prerequisites
            WHERE class_id = ?
        """, (new_class_id, class_id))
        
        # Copy exclusions
        cursor.execute("""
            INSERT INTO class_exclusions (
                class_id, exclusion_type, target_id,
                min_value, max_value
            )
            SELECT 
                ?, exclusion_type, target_id,
                min_value, max_value
            FROM class_exclusions
            WHERE class_id = ?
        """, (new_class_id, class_id))
        
        cursor.execute("COMMIT")
        return True, f"Class copied successfully as '{new_name}'", new_class_id
        
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error copying class: {str(e)}", None
    finally:
        conn.close()