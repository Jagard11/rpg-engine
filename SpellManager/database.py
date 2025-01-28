# ./SpellManager/database.py

import sqlite3
from typing import Dict, List, Optional
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_spell_tiers() -> List[Dict]:
    """Load all spell tiers from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT id, tier_name 
            FROM spell_tiers 
            ORDER BY tier_number
        """)
        
        tiers = [{"id": row[0], "tier_name": row[1]} for row in cursor.fetchall()]
        return tiers
        
    finally:
        conn.close()

def load_spell_type() -> List[Dict]:
    """Load all spell types from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT id, name
            FROM spell_type 
            ORDER BY name
        """)
        
        types = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
        return types
        
    finally:
        conn.close()

def load_spells() -> List[Dict]:
    """Load all spells from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT s.*, st.tier_name
            FROM spells s
            LEFT JOIN spell_tiers st ON s.spell_tier = st.id
            ORDER BY s.spell_tier, s.name
        """)
        
        columns = [col[0] for col in cursor.description]
        spells = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return spells
        
    finally:
        conn.close()

def save_spell(spell_data: Dict) -> bool:
    """Save spell to database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if spell_data.get('id'):
            cursor.execute("""
                UPDATE spells 
                SET name=?, spell_type_id=?, description=?, spell_tier=?, mp_cost=?,
                    casting_time=?, range=?, area_of_effect=?, duration=?
                WHERE id=?
            """, (
                spell_data['name'], 
                spell_data.get('spell_type_id'), 
                spell_data['description'],
                spell_data['spell_tier'], 
                spell_data['mp_cost'], 
                spell_data['casting_time'],
                spell_data['range'], 
                spell_data['area_of_effect'], 
                spell_data['duration'],
                spell_data['id']
            ))
        else:
            cursor.execute("""
                INSERT INTO spells (
                    name, spell_type_id, description, spell_tier, mp_cost,
                    casting_time, range, area_of_effect, duration
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_data['name'], 
                spell_data.get('spell_type_id'), 
                spell_data['description'],
                spell_data['spell_tier'], 
                spell_data['mp_cost'], 
                spell_data['casting_time'],
                spell_data['range'], 
                spell_data['area_of_effect'], 
                spell_data['duration']
            ))
            
        conn.commit()
        return True
        
    except Exception as e:
        print(f"Error saving spell: {str(e)}")
        return False
        
    finally:
        conn.close()

def load_spells_with_query(query: str) -> List[Dict]:
    """Load spells using custom SQL query"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute(query)
        columns = [col[0] for col in cursor.description]
        spells = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return spells
    finally:
        conn.close()

def delete_spell(spell_id: int) -> bool:
    """Delete spell from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("DELETE FROM spells WHERE id=?", (spell_id,))
        conn.commit()
        return True
    except Exception as e:
        print(f"Error deleting spell: {str(e)}")
        return False
    finally:
        conn.close()