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
                sc.cost_amount
            FROM spell_costs sc
            JOIN spells s ON sc.spell_id = s.id
            LEFT JOIN resources r ON sc.resource_id = r.id
            ORDER BY s.name
        """)
        columns = ['id', 'spell_name', 'spell_description', 'resource_name', 'cost_amount']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching spell wrappers: {e}")
        return []
    finally:
        conn.close()

def get_spell_wrapper_details(wrapper_id: int) -> Optional[Dict]:
    """Fetch details of a specific spell wrapper"""
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
                sc.cost_amount
            FROM spell_costs sc
            JOIN spells s ON sc.spell_id = s.id
            WHERE sc.id = ?
        """, (wrapper_id,))
        result = cursor.fetchone()
        if result:
            columns = ['id', 'spell_name', 'spell_description', 'spell_id', 'resource_id', 'cost_amount']
            return dict(zip(columns, result))
        return None
    except sqlite3.Error as e:
        st.error(f"Database error fetching wrapper details: {e}")
        return None
    finally:
        conn.close()

def save_spell_wrapper(data: Dict) -> Tuple[bool, str]:
    """Save or update a spell wrapper, handling nullable resource_id"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")

        # Ensure spell exists or create it
        cursor.execute("SELECT id FROM spells WHERE name = ?", (data['spell_name'],))
        spell_result = cursor.fetchone()
        if spell_result:
            spell_id = spell_result[0]
        else:
            cursor.execute("""
                INSERT INTO spells (name, description, spell_tier)
                VALUES (?, ?, 1)
            """, (data['spell_name'], data.get('spell_description', ''),))
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
        elif data.get('resource_id') is not None:  # Use existing resource_id if provided
            resource_id = data['resource_id']

        # Save or update spell_costs
        if data.get('id'):
            cursor.execute("""
                UPDATE spell_costs
                SET spell_id = ?, resource_id = ?, cost_amount = ?
                WHERE id = ?
            """, (spell_id, resource_id, data['cost_amount'], data['id']))
        else:
            cursor.execute("""
                INSERT INTO spell_costs (spell_id, resource_id, cost_amount)
                VALUES (?, ?, ?)
            """, (spell_id, resource_id, data['cost_amount']))
            data['id'] = cursor.lastrowid

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
        
        with st.form(key="spell_wrapper_form"):
            # Spells input: dropdown if records exist, text input if not
            spells = get_spells()
            if spells:
                spell_id = st.selectbox(
                    "Spell",
                    options=[s['id'] for s in spells],
                    format_func=lambda x: next(s['name'] for s in spells if s['id'] == x),
                    index=next((i for i, s in enumerate(spells) if s['id'] == wrapper_data.get('spell_id')), 0) if wrapper_data.get('spell_id') else 0
                )
                spell_name = next(s['name'] for s in spells if s['id'] == spell_id)
            else:
                st.warning("No spells found. Enter a new spell name below.")
                spell_name = st.text_input("New Spell Name", value=wrapper_data.get('spell_name', ''))

            # Resources input: dropdown with "None" option, or text input for new resource
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
                resource_name = st.text_input("New Resource Name (optional)", value=wrapper_data.get('resource_name', '') if wrapper_data.get('resource_name') else '')

            cost_amount = st.number_input("Cost Amount", min_value=0, value=wrapper_data.get('cost_amount', 0))
            spell_description = st.text_area("Spell Description (optional)", value=wrapper_data.get('spell_description', ''))

            submitted = st.form_submit_button(label="Save")
            if submitted:
                if not spell_name:
                    st.error("Spell name is required!")
                elif resource_name == '':
                    resource_name = None  # Explicitly set to None if empty
                else:
                    data = {
                        'id': wrapper_data.get('id'),
                        'spell_name': spell_name,
                        'spell_description': spell_description,
                        'resource_name': resource_name,
                        'cost_amount': cost_amount
                    }
                    if spells and 'spell_id' in locals():
                        data['spell_id'] = spell_id
                    if resources and resource_id is not None:
                        data['resource_id'] = resource_id
                    success, message = save_spell_wrapper(data)
                    if success:
                        st.success(message)
                        st.session_state.selected_wrapper_id = None
                        st.rerun()
                    else:
                        st.error(message)