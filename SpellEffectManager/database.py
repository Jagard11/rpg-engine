# ./SpellEffectManager/database.py

from utils.database import fetch_all, execute_transaction
from typing import List, Dict, Optional, Tuple

def get_spell_effects() -> List[Dict]:
    """Get list of all spell effects."""
    return fetch_all("""
        SELECT 
            se.id, se.name, et.name as effect_type_name, ms.name as magic_school_name
        FROM spell_effects se
        JOIN effect_types et ON se.effect_type_id = et.id
        JOIN magic_schools ms ON se.magic_school_id = ms.id
        ORDER BY se.name
    """)

def get_spell_effect_details(effect_id: int) -> Optional[Dict]:
    """Get full details of a specific spell effect."""
    effects = fetch_all("""
        SELECT 
            se.id, se.name, se.description, se.effect_type_id, se.magic_school_id,
            et.name as effect_type_name
        FROM spell_effects se
        JOIN effect_types et ON se.effect_type_id = et.id
        WHERE se.id = ?
    """, (effect_id,))
    if not effects:
        return None
    effect_data = effects[0]
    if effect_data['effect_type_name'] == 'damage':
        damage_data = fetch_all("""
            SELECT range_type_id, range_distance, base_damage, resistance_save_id
            FROM damage_effects
            WHERE spell_effect_id = ?
        """, (effect_id,))
        effect_data['damage_data'] = damage_data[0] if damage_data else {}
    return effect_data

def save_spell_effect(data: Dict) -> Tuple[bool, str]:
    """Save or update a spell effect."""
    try:
        if data.get('id'):
            execute_transaction("""
                UPDATE spell_effects 
                SET name=?, description=?, effect_type_id=?, magic_school_id=?
                WHERE id=?
            """, (data['name'], data['description'], data['effect_type_id'], data['magic_school_id'], data['id']))
        else:
            data['id'] = execute_transaction("""
                INSERT INTO spell_effects (name, description, effect_type_id, magic_school_id)
                VALUES (?, ?, ?, ?)
            """, (data['name'], data['description'], data['effect_type_id'], data['magic_school_id']))

        if data['effect_type_name'] == 'damage':
            execute_transaction("DELETE FROM damage_effects WHERE spell_effect_id=?", (data['id'],))
            execute_transaction("""
                INSERT INTO damage_effects (spell_effect_id, range_type_id, range_distance, base_damage, resistance_save_id)
                VALUES (?, ?, ?, ?, ?)
            """, (data['id'], data['damage_data']['range_type_id'], data['damage_data']['range_distance'],
                  data['damage_data']['base_damage'], data['damage_data']['resistance_save_id']))
        return True, f"Spell effect {'updated' if 'id' in data else 'created'} successfully!"
    except Exception as e:
        return False, f"Error saving spell effect: {str(e)}"

def get_reference_data(table: str) -> List[Dict]:
    """Fetch reference data from a table."""
    return fetch_all(f"SELECT id, name FROM {table} ORDER BY name")