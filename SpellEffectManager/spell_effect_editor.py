# ./SpellEffectManager/spell_effect_editor.py
import streamlit as st
import sqlite3
from typing import List, Dict, Optional, Tuple

def get_db_connection():
    conn = sqlite3.connect('rpg_data.db', isolation_level=None)  # Autocommit for simplicity
    conn.execute("PRAGMA foreign_keys = ON")  # Ensure FK constraints are enforced
    return conn

def get_spell_effects_list() -> List[Dict]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT se.id, se.name, et.name as effect_type, ms.name as magic_school
            FROM spell_effects se
            JOIN effect_types et ON se.effect_type_id = et.id
            JOIN magic_schools ms ON se.magic_school_id = ms.id
            ORDER BY se.name
        """)
        columns = ['id', 'name', 'effect_type', 'magic_school']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching effects: {e}")
        return []
    finally:
        conn.close()

def get_spell_effect_details(effect_id: int) -> Optional[Dict]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name, description, effect_type_id, magic_school_id
            FROM spell_effects
            WHERE id = ?
        """, (effect_id,))
        effect = cursor.fetchone()
        if effect:
            columns = ['id', 'name', 'description', 'effect_type_id', 'magic_school_id']
            return dict(zip(columns, effect))
        return None
    except sqlite3.Error as e:
        st.error(f"Database error fetching effect details: {e}")
        return None
    finally:
        conn.close()

def save_spell_effect(data: Dict) -> Tuple[bool, str]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        if data.get('id'):
            cursor.execute("""
                UPDATE spell_effects
                SET name = ?, description = ?, effect_type_id = ?, magic_school_id = ?
                WHERE id = ?
            """, (data['name'], data['description'], data['effect_type_id'], data['magic_school_id'], data['id']))
        else:
            cursor.execute("""
                INSERT INTO spell_effects (name, description, effect_type_id, magic_school_id)
                VALUES (?, ?, ?, ?)
            """, (data['name'], data['description'], data['effect_type_id'], data['magic_school_id']))
            data['id'] = cursor.lastrowid
        return True, f"Spell Effect {'updated' if data.get('id') else 'created'} successfully! (ID: {data.get('id', 'new')})"
    except sqlite3.Error as e:
        return False, f"Database error saving spell effect: {str(e)}"
    finally:
        conn.close()

def get_effect_types() -> List[Dict]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM effect_types ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching effect types: {e}")
        return []
    finally:
        conn.close()

def get_magic_schools() -> List[Dict]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM magic_schools ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching magic schools: {e}")
        return []
    finally:
        conn.close()

def get_damage_types() -> List[Dict]:
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM damage_types ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    except sqlite3.Error as e:
        st.error(f"Database error fetching damage types: {e}")
        return []
    finally:
        conn.close()

def render_spell_effect_editor():
    st.header("Spell Effect Editor")
    col1, col2 = st.columns([1, 2])

    with col1:
        st.subheader("Spell Effects")
        effects = get_spell_effects_list()
        if not effects and st.session_state.get('effects_fetched', False):
            st.warning("No spell effects found. Database may be empty or inaccessible.")
        st.session_state.effects_fetched = True
        effect_options = {f"{e['name']} ({e['effect_type']}, {e['magic_school']})": e['id'] for e in effects}
        effect_options["Create New"] = None
        selected_effect = st.selectbox("Select Spell Effect", options=list(effect_options.keys()))
        if selected_effect != st.session_state.get('last_selected_effect', ''):
            st.session_state.selected_effect_id = effect_options[selected_effect]
            st.session_state.last_selected_effect = selected_effect

    with col2:
        effect_data = get_spell_effect_details(st.session_state.get('selected_effect_id')) if st.session_state.get('selected_effect_id') else {}
        with st.form("spell_effect_form"):
            name = st.text_input("Name", value=effect_data.get('name', ''))
            description = st.text_area("Description", value=effect_data.get('description', ''))
            effect_types = get_effect_types()
            if not effect_types:
                st.error("No effect types available. Please populate the effect_types table.")
            effect_type_id = st.selectbox(
                "Effect Type",
                options=[et['id'] for et in effect_types],
                format_func=lambda x: next(et['name'] for et in effect_types if et['id'] == x),
                index=next((i for i, et in enumerate(effect_types) if et['id'] == effect_data.get('effect_type_id')), 0) if effect_types else 0
            )
            magic_schools = get_magic_schools()
            if not magic_schools:
                st.error("No magic schools available. Please populate the magic_schools table.")
            magic_school_id = st.selectbox(
                "Magic School",
                options=[ms['id'] for ms in magic_schools],
                format_func=lambda x: next(ms['name'] for ms in magic_schools if ms['id'] == x),
                index=next((i for i, ms in enumerate(magic_schools) if ms['id'] == effect_data.get('magic_school_id')), 0) if magic_schools else 0
            )
            damage_types = get_damage_types()
            damage_type = st.selectbox(
                "Damage Type (optional)",
                options=[None] + [dt['id'] for dt in damage_types],
                format_func=lambda x: "None" if x is None else next(dt['name'] for dt in damage_types if dt['id'] == x),
                index=0
            )
            base_damage = st.text_input("Base Damage Formula (optional)", value="")
            if st.form_submit_button("Save"):
                if not name:
                    st.error("Name is required!")
                elif not effect_type_id:
                    st.error("Effect Type is required!")
                elif not magic_school_id:
                    st.error("Magic School is required!")
                else:
                    data = {
                        'id': effect_data.get('id'),
                        'name': name,
                        'description': description,
                        'effect_type_id': effect_type_id,
                        'magic_school_id': magic_school_id
                    }
                    success, message = save_spell_effect(data)
                    if success:
                        st.success(message)
                        st.session_state.selected_effect_id = None
                        st.rerun()
                    else:
                        st.error(message)