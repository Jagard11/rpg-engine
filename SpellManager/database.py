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

def load_effect_types() -> List[Dict]:
    """Load all effect types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM effect_types 
            ORDER BY name
        """)
        
        columns = [col[0] for col in cursor.description]
        types = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return types
    finally:
        conn.close()

def save_effect(effect_data: Dict) -> Optional[int]:
    """Save effect to database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if effect_data.get('id'):
            cursor.execute("""
                UPDATE effects 
                SET name=?, effect_type_id=?, base_value=?, 
                    value_scaling=?, duration=?, tick_type=?,
                    description=?
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

def load_spell_effects(spell_id: int) -> List[Dict]:
    """Load effects for a specific spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT e.*, et.name as type_name 
            FROM effects e
            JOIN effect_types et ON e.effect_type_id = et.id
            JOIN spell_effects se ON e.id = se.effect_id
            WHERE se.spell_id = ?
            ORDER BY se.effect_order
        """, (spell_id,))
        
        columns = [col[0] for col in cursor.description]
        effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return effects
    finally:
        conn.close()

def save_spell_effects(spell_id: int, effects: List[Dict]) -> bool:
    """Save effects for a spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("DELETE FROM spell_effects WHERE spell_id = ?", (spell_id,))
        
        for idx, effect in enumerate(effects):
            cursor.execute("""
                INSERT INTO spell_effects (
                    spell_id, effect_id, effect_order, probability
                ) VALUES (?, ?, ?, ?)
            """, (
                spell_id,
                effect['id'],
                idx,
                effect.get('probability', 1.0)
            ))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell effects: {str(e)}")
        return False
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

def load_spell_requirements(spell_id: int) -> List[Dict]:
    """Load requirements for a specific spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_requirements 
            WHERE spell_id = ?
            ORDER BY requirement_group, id
        """, (spell_id,))
        
        columns = [col[0] for col in cursor.description]
        requirements = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return requirements
    finally:
        conn.close()

def save_spell_requirements(spell_id: int, requirements: List[Dict]) -> bool:
    """Save requirements for a spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("DELETE FROM spell_requirements WHERE spell_id = ?", (spell_id,))
        
        for req in requirements:
            if req.get('id'):
                cursor.execute("""
                    INSERT INTO spell_requirements (
                        id, spell_id, requirement_group, requirement_type,
                        target_type, target_value, comparison_type, value
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    req['id'], spell_id, req['requirement_group'],
                    req['requirement_type'], req['target_type'],
                    req['target_value'], req['comparison_type'],
                    req['value']
                ))
            else:
                cursor.execute("""
                    INSERT INTO spell_requirements (
                        spell_id, requirement_group, requirement_type,
                        target_type, target_value, comparison_type, value
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    spell_id, req['requirement_group'],
                    req['requirement_type'], req['target_type'],
                    req['target_value'], req['comparison_type'],
                    req['value']
                ))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell requirements: {str(e)}")
        return False
    finally:
        conn.close()

def load_spell_procedures(spell_id: int) -> List[Dict]:
    """Load procedures for a specific spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_procedures 
            WHERE spell_id = ?
            ORDER BY trigger_type, proc_order
        """, (spell_id,))
        
        columns = [col[0] for col in cursor.description]
        procedures = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return procedures
    finally:
        conn.close()

def save_spell_procedures(spell_id: int, procedures: List[Dict]) -> bool:
    """Save procedures for a spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("DELETE FROM spell_procedures WHERE spell_id = ?", (spell_id,))
        
        for proc in procedures:
            if proc.get('id'):
                cursor.execute("""
                    INSERT INTO spell_procedures (
                        id, spell_id, trigger_type, proc_order, action_type,
                        target_type, action_value, value_modifier, chance
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    proc['id'], spell_id, proc['trigger_type'],
                    proc['proc_order'], proc['action_type'],
                    proc['target_type'], proc['action_value'],
                    proc['value_modifier'], proc['chance']
                ))
            else:
                cursor.execute("""
                    INSERT INTO spell_procedures (
                        spell_id, trigger_type, proc_order, action_type,
                        target_type, action_value, value_modifier, chance
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    spell_id, proc['trigger_type'],
                    proc['proc_order'], proc['action_type'],
                    proc['target_type'], proc['action_value'],
                    proc['value_modifier'], proc['chance']
                ))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell procedures: {str(e)}")
        return False
    finally:
        conn.close()