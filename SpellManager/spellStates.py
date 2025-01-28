# ./SpellManager/spellStates.py

import sqlite3
from typing import Dict, List, Optional
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_spell_states() -> List[Dict]:
    """Load all spell states"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_states
            ORDER BY name
        """)
        
        columns = [col[0] for col in cursor.description]
        states = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return states
    finally:
        conn.close()

def save_spell_state(state_data: Dict) -> Optional[int]:
    """Save a spell state"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if state_data.get('id'):
            cursor.execute("""
                UPDATE spell_states 
                SET name=?, description=?, max_stacks=?, duration=?
                WHERE id=?
            """, (
                state_data['name'],
                state_data['description'],
                state_data['max_stacks'],
                state_data['duration'],
                state_data['id']
            ))
            state_id = state_data['id']
        else:
            cursor.execute("""
                INSERT INTO spell_states (
                    name, description, max_stacks, duration
                ) VALUES (?, ?, ?, ?)
            """, (
                state_data['name'],
                state_data['description'],
                state_data['max_stacks'],
                state_data['duration']
            ))
            state_id = cursor.lastrowid
            
        conn.commit()
        return state_id
    except Exception as e:
        print(f"Error saving spell state: {str(e)}")
        return None
    finally:
        conn.close()

def load_character_spell_states(character_id: int) -> List[Dict]:
    """Load all spell states for a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT css.*, ss.name, ss.description, ss.max_stacks
            FROM character_spell_states css
            JOIN spell_states ss ON css.spell_state_id = ss.id
            WHERE css.character_id = ?
            ORDER BY ss.name
        """, (character_id,))
        
        columns = [col[0] for col in cursor.description]
        states = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return states
    finally:
        conn.close()

def save_character_spell_state(character_id: int, state_data: Dict) -> bool:
    """Save a character's spell state"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if state_data.get('id'):
            cursor.execute("""
                UPDATE character_spell_states 
                SET current_stacks=?, remaining_duration=?, updated_at=CURRENT_TIMESTAMP
                WHERE id=? AND character_id=?
            """, (
                state_data['current_stacks'],
                state_data['remaining_duration'],
                state_data['id'],
                character_id
            ))
        else:
            cursor.execute("""
                INSERT INTO character_spell_states (
                    character_id, spell_state_id, current_stacks,
                    remaining_duration
                ) VALUES (?, ?, ?, ?)
            """, (
                character_id,
                state_data['spell_state_id'],
                state_data['current_stacks'],
                state_data['remaining_duration']
            ))
            
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving character spell state: {str(e)}")
        return False
    finally:
        conn.close()

def remove_character_spell_state(character_id: int, state_id: int) -> bool:
    """Remove a spell state from a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            DELETE FROM character_spell_states
            WHERE character_id=? AND id=?
        """, (character_id, state_id))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error removing character spell state: {str(e)}")
        return False
    finally:
        conn.close()

def get_state_effect_on_character(character_id: int, state_name: str) -> Optional[Dict]:
    """Get the effect of a specific state on a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT css.*, ss.name, ss.description, ss.max_stacks
            FROM character_spell_states css
            JOIN spell_states ss ON css.spell_state_id = ss.id
            WHERE css.character_id = ? AND ss.name = ?
        """, (character_id, state_name))
        
        row = cursor.fetchone()
        if row:
            columns = [col[0] for col in cursor.description]
            return dict(zip(columns, row))
        return None
    finally:
        conn.close()