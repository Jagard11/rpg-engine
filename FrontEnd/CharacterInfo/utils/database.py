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
                race_category_id,
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
        # Get character's race category and karma
        cursor.execute("""
            SELECT c.race_category_id, c.karma 
            FROM characters c 
            JOIN class_categories cc ON c.race_category_id = cc.id
            WHERE c.id = ?
        """, (character_id,))
        race_category_id, karma = cursor.fetchone()

        # Get available classes based on prerequisites
        cursor.execute("""
            SELECT DISTINCT
                c.id,
                c.name,
                c.description,
                t.name as type,
                c.is_racial,
                cat.name as category,
                subcat.name as subcategory,
                COALESCE(cp.current_level, 0) as current_level
            FROM classes c
            LEFT JOIN class_categories cat ON c.category_id = cat.id
            LEFT JOIN class_subcategories subcat ON c.subcategory_id = subcat.id
            LEFT JOIN class_types t ON c.class_type = t.id
            LEFT JOIN character_class_progression cp ON 
                cp.class_id = c.id AND cp.character_id = ?
            WHERE 
                (c.is_racial = FALSE OR cat.is_racial = TRUE AND cat.id = ?) 
                AND NOT EXISTS (
                    -- Check karma exclusions
                    SELECT 1 FROM class_exclusions ce
                    WHERE ce.class_id = c.id
                    AND ce.exclusion_type = 'karma'
                    AND ? BETWEEN ce.min_value AND ce.max_value
                )
                AND NOT EXISTS (
                    -- Check if any exclusions apply
                    SELECT 1 FROM class_exclusions ce2
                    WHERE ce2.class_id = c.id
                    AND ce2.exclusion_type IN ('specific_class', 'category_total', 'subcategory_total', 'racial_total')
                    AND EXISTS (
                        -- Complex exclusion logic would go here
                        -- For now, just exclude if there's any non-karma exclusion
                        SELECT 1 FROM character_class_progression cp2
                        WHERE cp2.character_id = ?
                    )
                )
                AND (
                    -- Allow classes with no prerequisites
                    NOT EXISTS (
                        SELECT 1 FROM class_prerequisites cp
                        WHERE cp.class_id = c.id
                    )
                    OR
                    -- Or check if all prerequisite groups are satisfied
                    EXISTS (
                        SELECT prerequisite_group
                        FROM class_prerequisites cp
                        WHERE cp.class_id = c.id
                        GROUP BY prerequisite_group
                        HAVING COUNT(*) = (
                            -- Count satisfied prerequisites per group
                            SELECT COUNT(*)
                            FROM class_prerequisites cp2
                            WHERE cp2.class_id = c.id
                            AND cp2.prerequisite_group = cp.prerequisite_group
                            AND (
                                -- Check karma prerequisites
                                (cp2.prerequisite_type = 'karma' AND ? BETWEEN cp2.min_value AND cp2.max_value)
                                OR 
                                -- Check class level prerequisites
                                (cp2.prerequisite_type = 'specific_class' AND EXISTS (
                                    SELECT 1 FROM character_class_progression cp3
                                    WHERE cp3.character_id = ?
                                    AND cp3.class_id = cp2.target_id
                                    AND cp3.current_level >= cp2.required_level
                                ))
                                -- Add more prerequisite type checks here
                            )
                        )
                    )
                )
            ORDER BY c.is_racial DESC, t.id, cat.name, c.name
        """, (character_id, race_category_id, karma, character_id, karma, character_id))

        columns = ['id', 'name', 'description', 'type', 'is_racial', 'category', 
                  'subcategory', 'current_level']
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
                t.name as type, 
                c.is_racial,
                cat.name as category,
                subcat.name as subcategory
            FROM classes c
            LEFT JOIN class_types t ON c.class_type = t.id
            LEFT JOIN class_categories cat ON c.category_id = cat.id
            LEFT JOIN class_subcategories subcat ON c.subcategory_id = subcat.id
            ORDER BY c.is_racial DESC, t.id, cat.name, c.name
        """)
        return cursor.fetchall()
    except Exception as e:
        raise Exception(f"Error loading available classes: {str(e)}")
    finally:
        conn.close()