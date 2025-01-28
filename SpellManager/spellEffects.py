# ./SpellManager/spellEffects.py
"""
Database handler for spell effects system. This module handles the creation,
reading, updating and deletion of spell effects and their relationships to spells.
"""

import sqlite3
from typing import Dict, List, Optional
from pathlib import Path

def init_effects_tables():
    """Initialize the effects tables if they don't exist"""
    conn = sqlite3.connect('rpg_data.db')
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
    
    # Create spell_effects junction table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS spell_effects (
        id INTEGER PRIMARY KEY,
        spell_id INTEGER NOT NULL,
        effect_id INTEGER NOT NULL,
        effect_order INTEGER NOT NULL,  -- Order in which effects are applied
        probability REAL DEFAULT 1.0,    -- Chance of effect applying (0.0-1.0)
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE,
        FOREIGN KEY (effect_id) REFERENCES effects(id)
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
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    cursor.execute("SELECT id, name, description FROM effect_types ORDER BY name")
    types = [{"id": row[0], "name": row[1], "description": row[2]} 
             for row in cursor.fetchall()]
    
    conn.close()
    return types

def load_effects() -> List[Dict]:
    """Load all effects"""
    conn = sqlite3.connect('rpg_data.db')
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
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT e.*, se.effect_order, se.probability 
        FROM effects e
        JOIN spell_effects se ON e.id = se.effect_id
        WHERE se.spell_id = ?
        ORDER BY se.effect_order
    """, (spell_id,))
    
    columns = [col[0] for col in cursor.description]
    effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
    
    conn.close()
    return effects

def save_effect(effect_data: Dict) -> Optional[int]:
    """Save an effect and return its ID"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        if effect_data.get('id'):
            cursor.execute("""
                UPDATE effects 
                SET name=?, effect_type_id=?, base_value=?, value_scaling=?,
                    duration=?, tick_type=?, description=?
                WHERE id=?
            """, (
                effect_data['name'], effect_data['effect_type_id'],
                effect_data['base_value'], effect_data['value_scaling'],
                effect_data['duration'], effect_data['tick_type'],
                effect_data['description'], effect_data['id']
            ))
            effect_id = effect_data['id']
        else:
            cursor.execute("""
                INSERT INTO effects (
                    name, effect_type_id, base_value, value_scaling,
                    duration, tick_type, description
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (
                effect_data['name'], effect_data['effect_type_id'],
                effect_data['base_value'], effect_data['value_scaling'],
                effect_data['duration'], effect_data['tick_type'],
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

def save_spell_effects(spell_id: int, effect_ids: List[Dict]) -> bool:
    """Save spell-effect relationships"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        # Delete existing relationships
        cursor.execute("DELETE FROM spell_effects WHERE spell_id = ?", (spell_id,))
        
        # Insert new relationships
        for idx, effect in enumerate(effect_ids):
            cursor.execute("""
                INSERT INTO spell_effects (
                    spell_id, effect_id, effect_order, probability
                ) VALUES (?, ?, ?, ?)
            """, (spell_id, effect['id'], idx, effect.get('probability', 1.0)))
            
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell effects: {str(e)}")
        return False
    finally:
        conn.close()