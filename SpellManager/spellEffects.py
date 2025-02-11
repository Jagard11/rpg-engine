# ./SpellManager/spellEffects.py
"""
Database handler for spell effects system. This module handles the creation,
reading, updating and deletion of spell effects and their relationships to spells.
"""

import sqlite3
from typing import Dict, List, Optional
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def init_effects_tables():
    """Initialize the effects tables if they don't exist"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    # Create effect types table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS effect_types (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        description TEXT
    )
    """)
    
    # Create effects table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS effects (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        effect_type_id INTEGER NOT NULL,
        base_value INTEGER,
        value_scaling TEXT,
        duration INTEGER,  -- Number of turns
        tick_type TEXT,   -- 'start_of_turn', 'end_of_turn', 'immediate'
        description TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (effect_type_id) REFERENCES effect_types(id)
    )
    """)
    
    # Create spell_effects junction table - exactly matching SpellEffectsStructure.sql
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS spell_effects (
        id INTEGER PRIMARY KEY,
        spell_id INTEGER NOT NULL,
        effect_order INTEGER NOT NULL DEFAULT 1,
        effect_type TEXT NOT NULL, 
        base_value INTEGER NOT NULL,
        scaling_stat_id INTEGER, 
        scaling_formula TEXT, 
        duration INTEGER DEFAULT 0, 
        tick_rate INTEGER DEFAULT 1, 
        proc_chance REAL DEFAULT 1.0,
        target_stat_id INTEGER NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
        FOREIGN KEY (target_stat_id) REFERENCES stat_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
        FOREIGN KEY (scaling_stat_id) REFERENCES stat_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION
    )
    """)
    
    # Insert default effect types if they don't exist
    default_types = [
        (1, 'damage', 'Deals damage to target'),
        (2, 'healing', 'Restores health to target'),
        (3, 'stat_modify', 'Modifies target stats'),
        (4, 'status', 'Applies a status condition'),
        (5, 'control', 'Controls target movement/actions'),
        (6, 'summon', 'Summons creatures/objects'),
    ]
    
    cursor.executemany("""
    INSERT OR IGNORE INTO effect_types (id, name, description)
    VALUES (?, ?, ?)
    """, default_types)
    
    conn.commit()
    conn.close()

def load_effect_types() -> List[Dict]:
    """Load all effect types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("SELECT id, name, description FROM effect_types ORDER BY name")
    types = [{"id": row[0], "name": row[1], "description": row[2]} 
             for row in cursor.fetchall()]
    
    conn.close()
    return types

def load_effects() -> List[Dict]:
    """Load all effects"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT e.*, et.name as type_name 
        FROM effects e
        JOIN effect_types et ON e.effect_type_id = et.id
        ORDER BY e.name
    """)
    
    columns = [col[0] for col in cursor.description]
    effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
    
    conn.close()
    return effects

def load_spell_effects(spell_id: int) -> List[Dict]:
    """Load all effects for a specific spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT se.*
        FROM spell_effects se
        WHERE se.spell_id = ?
        ORDER BY se.effect_order
    """, (spell_id,))
    
    columns = [col[0] for col in cursor.description]
    effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
    
    conn.close()
    return effects

def save_effect(effect_data: Dict) -> Optional[int]:
    """Save an effect and return its ID"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if effect_data.get('id'):
            cursor.execute("""
                UPDATE effects 
                SET name=?, effect_type_id=?, base_value=?, value_scaling=?,
                    duration=?, tick_type=?, description=?
                WHERE id=?
            """, (
                effect_data['name'],
                effect_data['effect_type_id'],
                effect_data['base_value'],
                effect_data['value_scaling'],
                effect_data['duration'],
                effect_data['tick_type'],
                effect_data['description'],
                effect_data['id']
            ))
            effect_id = effect_data['id']
        else:
            cursor.execute("""
                INSERT INTO effects (
                    name, effect_type_id, base_value, value_scaling,
                    duration, tick_type, description
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (
                effect_data['name'],
                effect_data['effect_type_id'],
                effect_data['base_value'],
                effect_data['value_scaling'],
                effect_data['duration'],
                effect_data['tick_type'],
                effect_data['description']
            ))
            effect_id = cursor.lastrowid
        
        conn.commit()
        return effect_id
    except Exception as e:
        print(f"Error saving effect: {str(e)}")
        return None
    finally:
        conn.close()

def save_spell_effects(spell_id: int, effects: List[Dict]) -> bool:
    """Save spell-effect relationships"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Delete existing relationships
        cursor.execute("DELETE FROM spell_effects WHERE spell_id = ?", (spell_id,))
        
        # Insert new relationships
        for idx, effect in enumerate(effects):
            cursor.execute("""
                INSERT INTO spell_effects (
                    spell_id, effect_type, base_value,
                    target_stat_id, scaling_stat_id, scaling_formula, 
                    duration, tick_rate, proc_chance, effect_order
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_id,
                effect['type_name'],  # Use the effect type name
                effect.get('base_value', 0),
                effect.get('target_stat_id', 1),  # Default to first stat type
                effect.get('scaling_stat_id'),
                effect.get('scaling_formula', ''),
                effect.get('duration', 0),
                effect.get('tick_rate', 1),
                effect.get('proc_chance', 1.0),
                idx + 1  # Order starts at 1
            ))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell effects: {str(e)}")
        print(f"Effect data: {effects}")
        return False
    finally:
        conn.close()