# ./SpellManager/database.py

import sqlite3
from typing import Dict, List, Optional
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

# Spell Core Functions
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

def save_spell(spell_data: Dict) -> Optional[int]:
    """Save spell to database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if spell_data.get('id'):
            cursor.execute("""
                UPDATE spells 
                SET name=?, description=?, spell_tier=?, is_super_tier=?,
                    mp_cost=?, casting_time=?, range=?, area_of_effect=?,
                    damage_base=?, damage_scaling=?, healing_base=?, healing_scaling=?,
                    status_effects=?, duration=?
                WHERE id=?
            """, (
                spell_data['name'], 
                spell_data['description'],
                spell_data['spell_tier'],
                spell_data.get('is_super_tier', False),
                spell_data['mp_cost'],
                spell_data['casting_time'],
                spell_data['range'],
                spell_data['area_of_effect'],
                spell_data.get('damage_base', 0),
                spell_data.get('damage_scaling', ''),
                spell_data.get('healing_base', 0),
                spell_data.get('healing_scaling', ''),
                spell_data.get('status_effects', ''),
                spell_data['duration'],
                spell_data['id']
            ))
        else:
            cursor.execute("""
                INSERT INTO spells (
                    name, description, spell_tier, is_super_tier,
                    mp_cost, casting_time, range, area_of_effect,
                    damage_base, damage_scaling, healing_base, healing_scaling,
                    status_effects, duration
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_data['name'], 
                spell_data['description'],
                spell_data['spell_tier'],
                spell_data.get('is_super_tier', False),
                spell_data['mp_cost'],
                spell_data['casting_time'],
                spell_data['range'],
                spell_data['area_of_effect'],
                spell_data.get('damage_base', 0),
                spell_data.get('damage_scaling', ''),
                spell_data.get('healing_base', 0),
                spell_data.get('healing_scaling', ''),
                spell_data.get('status_effects', ''),
                spell_data['duration']
            ))
            
        conn.commit()
        return cursor.lastrowid if not spell_data.get('id') else spell_data['id']
    except Exception as e:
        print(f"Error saving spell: {str(e)}")
        return None
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

# Spell Type and Tier Functions
def load_spell_type() -> List[Dict]:
    """Load all spell types"""
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

def load_spell_tiers() -> List[Dict]:
    """Load all spell tiers"""
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

# Effect Functions
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

def load_effects() -> List[Dict]:
    """Load all effects"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT e.*, et.name as type_name 
            FROM effects e
            JOIN effect_types et ON e.effect_type_id = et.id
            ORDER BY e.name
        """)
        
        columns = [col[0] for col in cursor.description]
        effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return effects
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

def load_spell_effects(spell_id: int) -> List[Dict]:
    """Load effects for a specific spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT se.id, se.spell_id, se.target_stat_id, se.effect_type,
                   se.base_value, se.scaling_stat_id, se.scaling_formula,
                   se.duration, se.tick_rate, se.proc_chance,
                   se.effect_order,
                   st.name as target_stat_name,
                   sst.name as scaling_stat_name
            FROM spell_effects se
            LEFT JOIN stat_types st ON se.target_stat_id = st.id
            LEFT JOIN stat_types sst ON se.scaling_stat_id = sst.id
            WHERE se.spell_id = ?
            ORDER BY se.effect_order
        """, (spell_id,))
        
        columns = [col[0] for col in cursor.description]
        effects = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return effects
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
        for idx, effect in enumerate(effects, start=1):
            # Print debug information
            print(f"Saving effect {idx}: {effect}")
            
            cursor.execute("""
                INSERT INTO spell_effects (
                    spell_id, effect_type, base_value,
                    target_stat_id, scaling_stat_id, scaling_formula, 
                    duration, tick_rate, proc_chance, effect_order
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_id,
                effect.get('effect_type', 'unknown'),
                effect.get('base_value', 0),
                effect.get('target_stat_id', 1),  # Default to first stat type
                effect.get('scaling_stat_id'),
                effect.get('scaling_formula', ''),
                effect.get('duration', 0),
                effect.get('tick_rate', 1),
                effect.get('proc_chance', 1.0),
                idx
            ))
        
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving spell effects: {str(e)}")
        print(f"Effect data: {effects}")
        conn.rollback()
        return False
    finally:
        conn.close()

# Spell Activation Functions
def load_spell_activation_types() -> List[Dict]:
    """Load all spell activation types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_activation_types
            ORDER BY name
        """)
        
        columns = [col[0] for col in cursor.description]
        types = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return types
    finally:
        conn.close()

def save_spell_activation_requirement(requirement_data: Dict) -> bool:
    """Save spell activation requirement"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if requirement_data.get('id'):
            cursor.execute("""
                UPDATE spell_activation_requirements 
                SET requirement_type=?, requirement_value=?, updated_at=CURRENT_TIMESTAMP
                WHERE id=?
            """, (
                requirement_data['requirement_type'],
                requirement_data['requirement_value'],
                requirement_data['id']
            ))
        else:
            cursor.execute("""
                INSERT INTO spell_activation_requirements (
                    spell_id, requirement_type, requirement_value
                ) VALUES (?, ?, ?)
            """, (
                requirement_data['spell_id'],
                requirement_data['requirement_type'],
                requirement_data['requirement_value']
            ))
            
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving activation requirement: {str(e)}")
        return False
    finally:
        conn.close()

def load_spell_activation_requirements(spell_id: int) -> List[Dict]:
    """Load activation requirements for a spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_activation_requirements
            WHERE spell_id = ?
            ORDER BY requirement_type
        """, (spell_id,))
        
        columns = [col[0] for col in cursor.description]
        requirements = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return requirements
    finally:
        conn.close()

# Spell Targeting Functions
def save_spell_targeting(targeting_data: Dict) -> bool:
    """Save spell targeting configuration"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if targeting_data.get('id'):
            # Update existing targeting config
            placeholders = ', '.join(f"{k}=?" for k in targeting_data.keys() if k != 'id')
            values = [v for k, v in targeting_data.items() if k != 'id']
            values.append(targeting_data['id'])
            
            cursor.execute(f"""
                UPDATE spell_targeting 
                SET {placeholders}, updated_at=CURRENT_TIMESTAMP
                WHERE id=?
            """, values)
        else:
            # Insert new targeting config
            keys = [k for k in targeting_data.keys() if k != 'id']
            placeholders = ', '.join('?' * len(keys))
            values = [targeting_data[k] for k in keys]
            
            cursor.execute(f"""
                INSERT INTO spell_targeting (
                    {', '.join(keys)}
                ) VALUES ({placeholders})
            """, values)
            
        conn.commit()
        return True
    except Exception as e:
        print(f"Error saving targeting configuration: {str(e)}")
        return False
    finally:
        conn.close()

def load_spell_targeting(spell_id: int) -> Optional[Dict]:
    """Load targeting configuration for a spell"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("""
            SELECT * FROM spell_targeting
            WHERE spell_id = ?
        """, (spell_id,))
        
        row = cursor.fetchone()
        if row:
            columns = [col[0] for col in cursor.description]
            return dict(zip(columns, row))
        return None
    finally:
        conn.close()

# Spell Requirements Functions
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

# Spell Procedures Functions
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