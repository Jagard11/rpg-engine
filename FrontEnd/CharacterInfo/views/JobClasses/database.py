# ./FrontEnd/CharacterInfo/views/JobClasses/database.py

import sqlite3
from typing import Dict, List, Optional, Tuple
from ...utils.database import get_db_connection

def get_class_types() -> List[Dict[str, any]]:
    """Get list of class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_types 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_categories() -> List[Dict[str, any]]:
    """Get list of non-racial class categories"""
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

def get_class_subcategories() -> List[Dict[str, any]]:
    """Get list of class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_subcategories 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_all_classes() -> List[Dict[str, any]]:
    """Get list of all classes for reference"""
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
            ORDER BY c.is_racial DESC, cc.name, c.name
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

def get_all_job_classes() -> List[Dict]:
    """Get all non-racial classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type,
                cc.name as category,
                cs.name as subcategory
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE c.is_racial = 0
            ORDER BY cc.name, c.name
        """)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_details(class_id: int) -> Optional[Dict]:
    """Get full details of a specific class"""
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
            WHERE c.id = ? AND c.is_racial = 0
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
                cp.prerequisite_group,
                cp.prerequisite_type,
                cp.target_id,
                cp.required_level,
                cp.min_value,
                cp.max_value,
                c2.name as target_name,
                cc.name as target_category,
                cs.name as target_subcategory
            FROM class_prerequisites cp
            LEFT JOIN classes c2 ON cp.target_id = c2.id
            LEFT JOIN class_categories cc ON cp.target_id = cc.id
            LEFT JOIN class_subcategories cs ON cp.target_id = cs.id
            WHERE cp.class_id = ?
            ORDER BY cp.prerequisite_group, cp.id
        """, (class_id,))
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_exclusions(class_id: int) -> List[Dict]:
    """Get exclusions for a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                ce.exclusion_type,
                ce.target_id,
                ce.min_value,
                ce.max_value,
                c2.name as target_name,
                cc.name as target_category,
                cs.name as target_subcategory
            FROM class_exclusions ce
            LEFT JOIN classes c2 ON ce.target_id = c2.id
            LEFT JOIN class_categories cc ON ce.target_id = cc.id
            LEFT JOIN class_subcategories cs ON ce.target_id = cs.id
            WHERE ce.class_id = ?
            ORDER BY ce.id
        """, (class_id,))
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def save_class(class_data: Dict, prerequisites: List[Dict], exclusions: List[Dict]) -> Tuple[bool, str]:
    """Save or update a class with its prerequisites and exclusions"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if class_data.get('id'):
            # Update existing class
            cursor.execute("""
                UPDATE classes 
                SET name = ?,
                    description = ?,
                    class_type = ?,
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
                class_data['class_type'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'],
                class_data['hp_per_level'], class_data['mp_per_level'],
                class_data['physical_attack_per_level'], class_data['physical_defense_per_level'],
                class_data['agility_per_level'], class_data['magical_attack_per_level'],
                class_data['magical_defense_per_level'], class_data['resistance_per_level'],
                class_data['special_per_level'],
                class_data['id']
            ))
            class_id = class_data['id']
        else:
            # Insert new class
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, is_racial,
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
                ) VALUES (?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'],
                class_data['hp_per_level'], class_data['mp_per_level'],
                class_data['physical_attack_per_level'], class_data['physical_defense_per_level'],
                class_data['agility_per_level'], class_data['magical_attack_per_level'],
                class_data['magical_defense_per_level'], class_data['resistance_per_level'],
                class_data['special_per_level']
            ))
            class_id = cursor.lastrowid

        # Delete existing prerequisites and exclusions
        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", (class_id,))
        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", (class_id,))

        # Save prerequisites
        for group_idx, group in enumerate(prerequisites):
            for prereq in group:
                cursor.execute("""
                    INSERT INTO class_prerequisites (
                        class_id, prerequisite_group, prerequisite_type,
                        target_id, required_level, min_value, max_value
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

        # Save exclusions
        for excl in exclusions:
            cursor.execute("""
                INSERT INTO class_exclusions (
                    class_id, exclusion_type, target_id,
                    min_value, max_value
                ) VALUES (?, ?, ?, ?, ?)
            """, (
                class_id,
                excl['type'],
                excl.get('target_id'),
                excl.get('min_value'),
                excl.get('max_value')
            ))

        cursor.execute("COMMIT")
        return True, f"Class {'updated' if class_data.get('id') else 'created'} successfully!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving class: {str(e)}"
    finally:
        conn.close()

def copy_class(class_id: int) -> Tuple[bool, str, Optional[int]]:
    """Copy a class and all its prerequisites and exclusions"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")

        # Get original class data
        cursor.execute("""
            SELECT * FROM classes 
            WHERE id = ? AND is_racial = 0
        """, (class_id,))
        original = cursor.fetchone()
        if not original:
            return False, "Class not found or is racial class", None

        # Create new name
        new_name = f"{original[1]} (Copy)"
        counter = 1
        while True:
            cursor.execute("SELECT COUNT(*) FROM classes WHERE name = ?", (new_name,))
            if cursor.fetchone()[0] == 0:
                break
            counter += 1
            new_name = f"{original[1]} (Copy {counter})"

        # Insert new class
        cursor.execute("""
            INSERT INTO classes (
                name, description, class_type, is_racial, category_id,
                subcategory_id, base_hp, base_mp, base_physical_attack,
                base_physical_defense, base_agility, base_magical_attack,
                base_magical_defense, base_resistance, base_special,
                hp_per_level, mp_per_level, physical_attack_per_level,
                physical_defense_per_level, agility_per_level,
                magical_attack_per_level, magical_defense_per_level,
                resistance_per_level, special_per_level
            ) SELECT 
                ?, description, class_type, is_racial, category_id,
                subcategory_id, base_hp, base_mp, base_physical_attack,
                base_physical_defense, base_agility, base_magical_attack,
                base_magical_defense, base_resistance, base_special,
                hp_per_level, mp_per_level, physical_attack_per_level,
                physical_defense_per_level, agility_per_level,
                magical_attack_per_level, magical_defense_per_level,
                resistance_per_level, special_per_level
            FROM classes WHERE id = ?
        """, (new_name, class_id))
        
        new_id = cursor.lastrowid

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
        """, (new_id, class_id))

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
        """, (new_id, class_id))

        cursor.execute("COMMIT")
        return True, f"Class copied as '{new_name}'", new_id
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error copying class: {str(e)}", None
    finally:
        conn.close()

def delete_class(class_id: int) -> Tuple[bool, str]:
    """Delete a job class and its prerequisites/exclusions"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Check if class exists and is non-racial
        cursor.execute("""
            SELECT COUNT(*) FROM classes 
            WHERE id = ? AND is_racial = 0
        """, (class_id,))
        if cursor.fetchone()[0] == 0:
            return False, "Class not found or is racial class"

        # Check if class is in use
        cursor.execute("""
            SELECT COUNT(*) FROM character_class_progression
            WHERE class_id = ?
        """, (class_id,))
        if cursor.fetchone()[0] > 0:
            return False, "Cannot delete: Class is currently used by one or more characters"

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