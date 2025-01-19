# ./FrontEnd/CharacterInfo/utils/database.py

import sqlite3
from typing import Optional, List, Dict, Tuple
from ..models.Character import Character

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def load_character(character_id: int) -> Optional[Character]:
    """Load a character's basic information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                id,
                first_name,
                middle_name,
                last_name,
                bio,
                total_level,
                birth_place,
                age,
                karma,
                talent,
                race_category,
                is_active,
                created_at,
                updated_at
            FROM characters 
            WHERE id = ?
        """, (character_id,))
        result = cursor.fetchone()
        if result:
            return Character(*result)
        return None
    except Exception as e:
        raise Exception(f"Error loading character: {str(e)}")
    finally:
        conn.close()

def load_character_classes(character_id: int) -> List[Dict]:
    """Load character's class information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.is_racial,
                cp.current_level as level,
                cp.current_experience as exp
            FROM character_class_progression cp
            JOIN classes c ON cp.class_id = c.id
            WHERE cp.character_id = ?
            ORDER BY c.is_racial DESC, c.name
        """, (character_id,))
        columns = ['id', 'name', 'is_racial', 'level', 'exp']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        raise Exception(f"Error loading character classes: {str(e)}")
    finally:
        conn.close()

def get_available_classes_for_level_up(character_id: int) -> List[Dict]:
    """Get list of available classes for level up"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Get character's race category
        cursor.execute("""
            SELECT race_category, karma 
            FROM characters 
            WHERE id = ?
        """, (character_id,))
        race_category, karma = cursor.fetchone()

        # Get available classes based on prerequisites and karma
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.description,
                c.class_type,
                c.is_racial,
                cat.name as category,
                subcat.name as subcategory,
                c.karma_requirement_min,
                c.karma_requirement_max,
                COALESCE(cp.current_level, 0) as current_level
            FROM classes c
            LEFT JOIN class_categories cat ON c.category_id = cat.id
            LEFT JOIN class_subcategories subcat ON c.subcategory_id = subcat.id
            LEFT JOIN character_class_progression cp ON 
                cp.class_id = c.id AND cp.character_id = ?
            WHERE 
                (c.is_racial = FALSE OR cat.is_racial = TRUE AND cat.name = ?) AND
                ? BETWEEN c.karma_requirement_min AND c.karma_requirement_max AND
                NOT EXISTS (
                    SELECT 1 
                    FROM class_prerequisites p
                    WHERE p.class_id = c.id AND
                    NOT EXISTS (
                        SELECT 1 
                        FROM character_class_progression cp2
                        WHERE cp2.character_id = ? AND
                        cp2.class_id = p.required_class_id AND
                        cp2.current_level >= p.required_level
                    )
                )
            ORDER BY c.is_racial DESC, c.class_type, cat.name, c.name
        """, (character_id, race_category, karma, character_id))

        columns = ['id', 'name', 'description', 'type', 'is_racial', 'category', 
                  'subcategory', 'karma_min', 'karma_max', 'current_level']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def can_change_race_category(character_id: int) -> bool:
    """Check if character can change race category"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT COUNT(*) 
            FROM character_class_progression cp
            JOIN classes c ON cp.class_id = c.id
            WHERE cp.character_id = ? AND c.is_racial = TRUE
        """, (character_id,))
        count = cursor.fetchone()[0]
        return count == 0
    finally:
        conn.close()

def load_available_classes() -> List[tuple]:
    """Load list of available classes from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id, 
                c.name, 
                c.description, 
                c.class_type, 
                c.is_racial,
                cat.name as category,
                subcat.name as subcategory
            FROM classes c
            LEFT JOIN class_categories cat ON c.category_id = cat.id
            LEFT JOIN class_subcategories subcat ON c.subcategory_id = subcat.id
            ORDER BY c.is_racial DESC, c.class_type, cat.name, c.name
        """)
        return cursor.fetchall()
    except Exception as e:
        raise Exception(f"Error loading available classes: {str(e)}")
    finally:
        conn.close()