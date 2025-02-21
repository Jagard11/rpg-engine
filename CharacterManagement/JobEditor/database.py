# ./CharacterManagement/JobEditor/database.py

import sqlite3
from typing import Dict, List, Optional, Tuple

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def get_job_classes(
    search: str = "",
    type_filter: str = "All",
    category_filter: str = "All"
) -> List[Dict]:
    """Get filtered list of job classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Build query conditions
        conditions = ["c.is_racial = FALSE"]
        params = []

        if search:
            conditions.append("(c.name LIKE ? OR c.description LIKE ?)")
            search_term = f"%{search}%"
            params.extend([search_term, search_term])

        if type_filter != "All":
            conditions.append("ct.name = ?")
            params.append(type_filter)

        if category_filter != "All":
            conditions.append("cc.name = ?")
            params.append(category_filter)

        # Construct and execute query
        query = f"""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name,
                c.base_hp,
                c.base_mp,
                c.hp_per_level,
                c.mp_per_level
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE {" AND ".join(conditions)}
            ORDER BY cc.name, c.name
        """
        
        cursor.execute(query, params)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_types() -> List[Dict]:
    """Get list of available class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT DISTINCT ct.id, ct.name
            FROM class_types ct
            JOIN classes c ON ct.id = c.class_type
            WHERE c.is_racial = FALSE
            ORDER BY ct.name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_categories() -> List[Dict]:
    """Get list of available job class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT DISTINCT cc.id, cc.name
            FROM class_categories cc
            JOIN classes c ON cc.id = c.category_id
            WHERE c.is_racial = FALSE
            ORDER BY cc.name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_subcategories() -> List[Dict]:
    """Get list of available job class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT DISTINCT cs.id, cs.name
            FROM class_subcategories cs
            JOIN classes c ON cs.id = c.subcategory_id
            WHERE c.is_racial = FALSE
            ORDER BY cs.name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def save_job_class(class_data: Dict) -> Tuple[bool, str]:
    """Save or update a job class"""
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
                    category_id = ?,
                    subcategory_id = ?,
                    base_hp = ?,
                    base_mp = ?,
                    hp_per_level = ?,
                    mp_per_level = ?
                WHERE id = ? AND is_racial = FALSE
            """, (
                class_data['name'],
                class_data['description'],
                class_data['class_type'],
                class_data['category_id'],
                class_data['subcategory_id'],
                class_data['base_hp'],
                class_data['base_mp'],
                class_data['hp_per_level'],
                class_data['mp_per_level'],
                class_data['id']
            ))
        else:
            # Insert new class
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, is_racial, category_id, subcategory_id,
                    base_hp, base_mp, hp_per_level, mp_per_level
                ) VALUES (?, ?, ?, FALSE, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'],
                class_data['description'],
                class_data['class_type'],
                class_data['category_id'],
                class_data['subcategory_id'],
                class_data['base_hp'],
                class_data['base_mp'],
                class_data['hp_per_level'],
                class_data['mp_per_level']
            ))

        conn.commit()
        return True, "Job class saved successfully!"
    except Exception as e:
        conn.rollback()
        return False, f"Error saving job class: {str(e)}"
    finally:
        conn.close()