# ./SpellEffectManager/spell_wrappers.py

import streamlit as st
import sqlite3
from typing import List, Dict, Optional, Tuple

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def get_spell_wrappers() -> List[Dict]:
    """Fetch all spell wrappers with spell names and resource info"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                sc.id, 
                s.name AS spell_name, 
                s.description AS spell_description, 
                r.name AS resource_name, 
                sc.cost_amount,
                s.charges_per_day,
                st.max_range,
                st.max_targets
            FROM spell_costs sc
            JOIN spells s ON sc.spell_id = s.id
            LEFT JOIN resources r ON sc.resource_id = r.id
            LEFT JOIN spell_targeting st ON s.id = st.spell_id
            ORDER BY s.name
        """)
        columns = ['id', 'spell_name', 'spell_description', 'resource_name', 'cost_amount', 
                   'charges_per_day', 'max_range', 'max_targets']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching spell wrappers: {e}")
        return []
    finally:
        conn.close()

def get_spell_wrapper_details(wrapper_id: int) -> Optional[Dict]:
    """Fetch details of a specific spell wrapper, including associated effects"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                sc.id, 
                s.name AS spell_name, 
                s.description AS spell_description, 
                sc.spell_id, 
                sc.resource_id, 
                sc.cost_amount,
                s.charges_per_day,
                st.max_range,
                st.max_targets,
                st.requires_los,
                st.allow_dead_targets,
                st.ignore_target_immunity
            FROM spell_costs sc
            JOIN spells s ON sc.spell_id = s.id
            LEFT JOIN spell_targeting st ON s.id = st.spell_id
            WHERE sc.id = ?
        """, (wrapper_id,))
        result = cursor.fetchone()
        if not result:
            return None
        columns = ['id', 'spell_name', 'spell_description', 'spell_id', 'resource_id', 
                   'cost_amount', 'charges_per_day', 'max_range', 'max_targets', 
                   'requires_los', 'allow_dead_targets', 'ignore_target_immunity']
        wrapper_data = dict(zip(columns, result))

        cursor.execute("""
            SELECT se.id, se.name
            FROM spell_has_effects she
            JOIN spell_effects se ON she.spell_effect_id = se.id
            WHERE she.spell_id = ?
            ORDER BY she.effect_order
        """, (wrapper_data['spell_id'],))
        effects = [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
        wrapper_data['effect_ids'] = [e['id'] for e in effects]
        return wrapper_data
    except sqlite3.Error as e:
        st.error(f"Database error fetching wrapper details: {e}")
        return None
    finally:
        conn.close()

def save_spell_wrapper(data: Dict) -> Tuple[bool, str]:
    """Save or update a spell wrapper"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")

        # Ensure spell exists or create it
        cursor.execute("SELECT id FROM spells WHERE name = ?", (data['spell_name'],))
        spell_result = cursor.fetchone()
        if spell_result:
            spell_id = spell_result[0]
            cursor.execute("""
                UPDATE spells SET description = ?, charges_per_day = ?
                WHERE id = ?
            """, (data['spell_description'], data['charges_per_day'], spell_id))
        else:
            cursor.execute("""
                INSERT INTO spells (name, description, spell_tier, charges_per_day)
                VALUES (?, ?, 1, ?)
            """, (data['spell_name'], data['spell_description'], data['charges_per_day']))
            spell_id = cursor.lastrowid

        # Handle resource (optional)
        resource_id = None
        if data.get('resource_name'):
            cursor.execute("SELECT id FROM resources WHERE name = ?", (data['resource_name'],))
            resource_result = cursor.fetchone()
            if resource_result:
                resource_id = resource_result[0]
            else:
                cursor.execute("""
                    INSERT INTO resources (name, description)
                    VALUES (?, ?)
                """, (data['resource_name'], ''))
                resource_id = cursor.lastrowid
        elif data.get('resource_id') is not None:
            resource_id = data['resource_id']

        # Save or update spell_costs
        if data.get('id'):
            cursor.execute("""
                UPDATE spell_costs
                SET spell_id = ?, resource_id = ?, cost_amount = ?
                WHERE id = ?
            """, (spell_id, resource_id, data['cost_amount'], data['id']))
            wrapper_id = data['id']
        else:
            cursor.execute("""
                INSERT INTO spell_costs (spell_id, resource_id, cost_amount)
                VALUES (?, ?, ?)
            """, (spell_id, resource_id, data['cost_amount']))
            wrapper_id = cursor.lastrowid

        # Update spell_has_effects
        cursor.execute("DELETE FROM spell_has_effects WHERE spell_id = ?", (spell_id,))
        if data.get('effect_ids'):
            for order, effect_id in enumerate(data['effect_ids'], 1):
                cursor.execute("""
                    INSERT INTO spell_has_effects (spell_id, spell_effect_id, effect_order)
                    VALUES (?, ?, ?)
                """, (spell_id, effect_id, order))

        # Update spell_targeting
        cursor.execute("DELETE FROM spell_targeting WHERE spell_id = ?", (spell_id,))
        cursor.execute("""
            INSERT INTO spell_targeting (spell_id, max_targets, requires_los, allow_dead_targets, 
                                       ignore_target_immunity, max_range)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (spell_id, data['max_targets'], data['requires_los'], data['allow_dead_targets'], 
              data['ignore_target_immunity'], data['max_range']))

        conn.commit()
        return True, f"Spell Wrapper {'updated' if data.get('id') else 'created'} successfully!"
    except sqlite3.Error as e:
        conn.rollback()
        return False, f"Error saving spell wrapper: {str(e)}"
    finally:
        conn.close()

def get_spells() -> List[Dict]:
    """Fetch available spells"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM spells ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_resources() -> List[Dict]:
    """Fetch available resources"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM resources ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_spell_effects() -> List[Dict]:
    """Fetch available spell effects"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM spell_effects ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def render_spell_wrappers():
    """Render the spell wrappers editor"""
    st.header("Spell Wrappers Editor")
    col1, col2 = st.columns([1, 2])

    with col1:
        st.subheader("Spell Wrappers")
        wrappers = get_spell_wrappers()
        if not wrappers:
            st.info("No spell wrappers found yet. Use the form to create one.")
        wrapper_options = {f"{w['spell_name']} ({w['resource_name'] or 'No Resource'}, {w['cost_amount']})": w['id'] for w in wrappers}
        wrapper_options["Create New"] = None
        selected_wrapper = st.selectbox("Select Spell Wrapper", options=list(wrapper_options.keys()))
        if selected_wrapper != st.session_state.get('last_selected_wrapper', ''):
            st.session_state.selected_wrapper_id = wrapper_options[selected_wrapper]
            st.session_state.last_selected_wrapper = selected_wrapper

    with col2:
        wrapper_data = get_spell_wrapper_details(st.session_state.get('selected_wrapper_id')) if st.session_state.get('selected_wrapper_id') else {}
        
        with st.form("spell_wrapper_form"):
            spell_name = st.text_input("Spell Name", 
                                     value=wrapper_data.get('spell_name', ''),
                                     help="Enter the name of the spell (required).")
            spell_description = st.text_area("Spell Description (optional)", 
                                           value=wrapper_data.get('spell_description', ''))

            # Casting Time
            casting_time = st.number_input("Casting Time (seconds)", 
                                         min_value=0.0, step=0.1, 
                                         value=float(wrapper_data.get('casting_time', 0.0)) if wrapper_data.get('casting_time') is not None else 0.0)

            # Resources (optional)
            resources = get_resources()
            if resources:
                resource_options = [{'id': None, 'name': 'None'}] + resources
                resource_id = st.selectbox(
                    "Resource Cost Type (optional)",
                    options=[r['id'] for r in resource_options],
                    format_func=lambda x: next(r['name'] for r in resource_options if r['id'] == x),
                    index=next((i for i, r in enumerate(resource_options) if r['id'] == wrapper_data.get('resource_id')), 0) if wrapper_data.get('resource_id') is not None else 0
                )
                resource_name = None if resource_id is None else next(r['name'] for r in resources if r['id'] == resource_id)
            else:
                st.info("No resources found. Optionally enter a new resource name or leave blank.")
                resource_name = st.text_input("New Resource Name (optional)", 
                                            value=wrapper_data.get('resource_name', '') if wrapper_data.get('resource_name') else '')

            cost_amount = st.number_input("Cost Amount", 
                                        min_value=0, 
                                        value=wrapper_data.get('cost_amount', 0))

            # Charges Per Day
            charges_per_day = st.number_input("Charges Per Day (optional, 0 for unlimited)", 
                                            min_value=0, 
                                            value=wrapper_data.get('charges_per_day', 0) if wrapper_data.get('charges_per_day') is not None else 0)

            # Targeting Details
            st.subheader("Targeting")
            max_range = st.number_input("Max Range (meters)", 
                                      min_value=0, 
                                      value=wrapper_data.get('max_range', 0) if wrapper_data.get('max_range') is not None else 0)
            max_targets = st.number_input("Max Targets", 
                                        min_value=1, 
                                        value=wrapper_data.get('max_targets', 1) if wrapper_data.get('max_targets') is not None else 1)
            requires_los = st.checkbox("Requires Line of Sight", 
                                     value=wrapper_data.get('requires_los', True) if wrapper_data.get('requires_los') is not None else True)
            allow_dead_targets = st.checkbox("Allow Dead Targets", 
                                           value=wrapper_data.get('allow_dead_targets', False) if wrapper_data.get('allow_dead_targets') is not None else False)
            ignore_target_immunity = st.checkbox("Ignore Target Immunity", 
                                               value=wrapper_data.get('ignore_target_immunity', False) if wrapper_data.get('ignore_target_immunity') is not None else False)

            # Spell Effects
            effects = get_spell_effects()
            if effects:
                effect_ids = st.multiselect(
                    "Spell Effects",
                    options=[e['id'] for e in effects],
                    format_func=lambda x: next(e['name'] for e in effects if e['id'] == x),
                    default=wrapper_data.get('effect_ids', []),
                    help="Select one or more effects this spell will trigger."
                )
            else:
                st.info("No spell effects found. Create effects in the Spell Effect Editor first.")
                effect_ids = []

            submitted = st.form_submit_button(label="Save")
            if submitted:
                if not spell_name:
                    st.error("Spell name is required!")
                else:
                    if resource_name == '':
                        resource_name = None
                    data = {
                        'id': wrapper_data.get('id'),
                        'spell_name': spell_name,
                        'spell_description': spell_description,
                        'resource_name': resource_name,
                        'cost_amount': cost_amount,
                        'charges_per_day': charges_per_day if charges_per_day > 0 else None,
                        'casting_time': casting_time,
                        'max_range': max_range,
                        'max_targets': max_targets,
                        'requires_los': requires_los,
                        'allow_dead_targets': allow_dead_targets,
                        'ignore_target_immunity': ignore_target_immunity,
                        'effect_ids': effect_ids
                    }
                    if resources and resource_id is not None:
                        data['resource_id'] = resource_id
                    success, message = save_spell_wrapper(data)
                    if success:
                        st.success(message)
                        st.session_state.selected_wrapper_id = None
                        st.rerun()
                    else:
                        st.error(message)