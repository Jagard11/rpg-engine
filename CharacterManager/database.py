# ./CharacterManager/database.py

import sqlite3
from typing import List, Dict, Optional, Tuple

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def get_characters() -> List[Dict]:
    """Get list of all characters"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                COALESCE(c.first_name || ' ' || c.last_name, c.first_name) as name,
                c.total_level,
                cc.name as race_category_name
            FROM characters c
            LEFT JOIN class_categories cc ON c.race_category_id = cc.id
            WHERE c.is_active = TRUE
            ORDER BY c.first_name, c.last_name
        """)
        columns = ['id', 'name', 'total_level', 'race_category_name']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_character_details(character_id: int) -> Optional[Dict]:
    """Get full details of a specific character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.*,
                cc.name as race_category_name
            FROM characters c
            LEFT JOIN class_categories cc ON c.race_category_id = cc.id
            WHERE c.id = ? AND c.is_active = TRUE
        """, (character_id,))
        
        result = cursor.fetchone()
        if result:
            columns = [desc[0] for desc in cursor.description]
            return dict(zip(columns, result))
        return None
    finally:
        conn.close()

def get_character_classes(character_id: int) -> List[Dict]:
    """Get character's class progressions"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.is_racial,
                cp.current_level,
                cp.current_experience,
                cc.name as category_name
            FROM character_class_progression cp
            JOIN classes c ON cp.class_id = c.id
            LEFT JOIN class_categories cc ON c.category_id = cc.id
            WHERE cp.character_id = ?
            ORDER BY c.is_racial DESC, c.name
        """, (character_id,))
        
        columns = ['id', 'name', 'is_racial', 'level', 'experience', 'category_name']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def save_character(character_data: Dict) -> Tuple[bool, str]:
    """Save or update a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if character_data.get('id'):
            # Update existing character
            cursor.execute("""
                UPDATE characters 
                SET first_name = ?,
                    middle_name = ?,
                    last_name = ?,
                    bio = ?,
                    birth_place = ?,
                    age = ?,
                    race_category_id = ?,
                    talent = ?
                WHERE id = ? AND is_active = TRUE
            """, (
                character_data['first_name'],
                character_data.get('middle_name'),
                character_data.get('last_name'),
                character_data.get('bio'),
                character_data.get('birth_place'),
                character_data.get('age'),
                character_data['race_category_id'],
                character_data.get('talent'),
                character_data['id']
            ))
        else:
            # Insert new character
            cursor.execute("""
                INSERT INTO characters (
                    first_name, middle_name, last_name,
                    bio, birth_place, age,
                    race_category_id, talent,
                    total_level, karma, is_active
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, 0, TRUE)
            """, (
                character_data['first_name'],
                character_data.get('middle_name'),
                character_data.get('last_name'),
                character_data.get('bio'),
                character_data.get('birth_place'),
                character_data.get('age'),
                character_data['race_category_id'],
                character_data.get('talent')
            ))

        cursor.execute("COMMIT")
        return True, f"Character {'updated' if character_data.get('id') else 'created'} successfully!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving character: {str(e)}"
    finally:
        conn.close()

def delete_character(character_id: int) -> Tuple[bool, str]:
    """Soft delete a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            UPDATE characters 
            SET is_active = FALSE 
            WHERE id = ? AND is_active = TRUE
        """, (character_id,))
        
        if cursor.rowcount == 0:
            return False, "Character not found"
            
        conn.commit()
        return True, "Character deleted successfully"
    except Exception as e:
        return False, f"Error deleting character: {str(e)}"
    finally:
        conn.close()

def get_available_race_categories() -> List[Dict]:
    """Get list of available race categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = TRUE 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()