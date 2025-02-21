# ./ClassManager/classEditor.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path
from typing import Optional, Dict, Any
from datetime import datetime

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def get_foreign_key_options(table_name: str, name_field: str = 'name') -> Dict[int, str]:
    """Get options for foreign key dropdown menus"""
    query = f"SELECT id, {name_field} FROM {table_name}"
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn)
            return dict(zip(df['id'], df[name_field]))
    except Exception as e:
        st.error(f"Error loading {table_name}: {e}")
        return {}

def load_class_record(class_id: int) -> Optional[Dict[str, Any]]:
    """Load a specific class record"""
    if class_id == 0:
        return None
    query = "SELECT * FROM classes WHERE id = ?"
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[class_id])
            if not df.empty:
                return df.iloc[0].to_dict()
    except Exception as e:
        st.error(f"Error loading class record: {e}")
    return None

def get_class_spell_schools(class_id: int) -> set:
    """Get magic schools from assigned spells"""
    query = """
    SELECT DISTINCT ms.name
    FROM class_spell_lists csl
    JOIN spells s ON csl.spell_id = s.id
    JOIN spell_has_effects she ON s.id = she.spell_id
    JOIN spell_effects se ON she.spell_effect_id = se.id
    JOIN magic_schools ms ON se.magic_school_id = ms.id
    WHERE csl.class_id = ?
    """
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[class_id])
            return set(df['name'])
    except Exception as e:
        st.error(f"Error fetching spell schools: {e}")
        return set()

def save_class_record(record_data: Dict[str, Any], is_new: bool = True) -> bool:
    """Save the class record to the database"""
    if is_new:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        placeholders = ','.join(['?' for _ in columns])
        query = f"INSERT INTO classes ({','.join(columns)}) VALUES ({placeholders})"
        values = [record_data[col] for col in columns]
    else:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        set_clause = ','.join([f"{col} = ?" for col in columns])
        query = f"UPDATE classes SET {set_clause}, updated_at = CURRENT_TIMESTAMP WHERE id = ?"
        values = [record_data[col] for col in columns] + [record_data['id']]
    try:
        with get_db_connection() as conn:
            cursor = conn.execute(query, values)
            conn.commit()
            if is_new:
                record_data['id'] = cursor.lastrowid
            return True
    except Exception as e:
        st.error(f"Error saving record: {e}")
        return False

def delete_class_record(class_id: int) -> bool:
    """Delete a class record"""
    if class_id == 0:
        return False
    query = "DELETE FROM classes WHERE id = ?"
    try:
        with get_db_connection() as conn:
            conn.execute(query, [class_id])
            conn.commit()
            return True
    except Exception as e:
        st.error(f"Error deleting record: {e}")
        return False

def render_class_editor():
    """Render the class editor interface"""
    st.header("Class Editor")

    # Load foreign key options
    class_types = get_foreign_key_options('class_types')
    categories = get_foreign_key_options('class_categories')
    subcategories = get_foreign_key_options('class_subcategories')

    # Initialize or get current record ID
    if 'current_class_id' not in st.session_state:
        st.session_state.current_class_id = 0

    # Load record if selected from table view
    current_record = {}
    if 'selected_class_name' in st.session_state and 'class_query_results' in st.session_state:
        selected_name = st.session_state.selected_class_name
        if selected_name:
            selected_record = st.session_state.class_query_results[
                st.session_state.class_query_results['name'] == selected_name
            ]
            if not selected_record.empty:
                st.session_state.current_class_id = int(selected_record.iloc[0]['id'])
                current_record = load_class_record(st.session_state.current_class_id) or {}
    else:
        current_record = load_class_record(st.session_state.current_class_id) or {}

    with st.form("class_editor_form", clear_on_submit=False):
        ### Basic Information
        st.subheader("Basic Information")
        st.number_input("ID", value=st.session_state.current_class_id, disabled=True, key="class_id_input")
        name = st.text_input("Name", value=current_record.get('name', ''), key="class_name_input")
        description = st.text_area("Description", value=current_record.get('description', ''), key="class_description_input")

        current_class_type = current_record.get('class_type', list(class_types.keys())[0] if class_types else None)
        class_type = st.selectbox(
            "Class Type",
            options=list(class_types.keys()),
            format_func=lambda x: class_types.get(x, ''),
            key="class_type_input",
            index=list(class_types.keys()).index(current_class_type) if current_class_type in class_types else 0,
            help="Suggested: Base (15 levels), High (10 levels), Rare (5 levels)"
        )
        current_category = current_record.get('category_id', list(categories.keys())[0] if categories else None)
        category = st.selectbox(
            "Category",
            options=list(categories.keys()),
            format_func=lambda x: categories.get(x, ''),
            key="category_id_input",
            index=list(categories.keys()).index(current_category) if current_category in categories else 0
        )
        current_subcategory = current_record.get('subcategory_id', list(subcategories.keys())[0] if subcategories else None)
        subcategory = st.selectbox(
            "Subcategory",
            options=list(subcategories.keys()),
            format_func=lambda x: subcategories.get(x, ''),
            key="subcategory_id_input",
            index=list(subcategories.keys()).index(current_subcategory) if current_subcategory in subcategories else 0,
            help="For Race Classes, may represent creature type (e.g., Humanoid, Undead)"
        )
        is_racial = st.checkbox("Is Racial Class", value=current_record.get('is_racial', False), key="is_racial_input")

        if st.session_state.current_class_id != 0:
            spell_schools = get_class_spell_schools(st.session_state.current_class_id)
            if spell_schools:
                st.write(f"Spell Schools (from assigned spells): {', '.join(spell_schools)}")
            else:
                st.write("No spells assigned yet. Add via Spell List Editor.")

        ### Stats
        st.subheader("Starting Stats (Level 1)")
        col1, col2, col3 = st.columns(3)
        with col1:
            base_hp = st.number_input("Base HP", value=current_record.get('base_hp', 0), key="base_hp_input")
            base_mp = st.number_input("Base MP", value=current_record.get('base_mp', 0), key="base_mp_input")
            base_physical_attack = st.number_input("Base Physical Attack", value=current_record.get('base_physical_attack', 0), key="base_physical_attack_input")
        with col2:
            base_physical_defense = st.number_input("Base Physical Defense", value=current_record.get('base_physical_defense', 0), key="base_physical_defense_input")
            base_agility = st.number_input("Base Agility", value=current_record.get('base_agility', 0), key="base_agility_input")
            base_magical_attack = st.number_input("Base Magical Attack", value=current_record.get('base_magical_attack', 0), key="base_magical_attack_input")
        with col3:
            base_magical_defense = st.number_input("Base Magical Defense", value=current_record.get('base_magical_defense', 0), key="base_magical_defense_input")
            base_resistance = st.number_input("Base Resistance", value=current_record.get('base_resistance', 0), key="base_resistance_input")
            base_special = st.number_input("Base Special", value=current_record.get('base_special', 0), key="base_special_input")

        st.subheader("Stats Per Level (Beyond Level 1)")
        col1, col2, col3 = st.columns(3)
        with col1:
            hp_per_level = st.number_input("HP per Level", value=current_record.get('hp_per_level', 0), key="hp_per_level_input")
            mp_per_level = st.number_input("MP per Level", value=current_record.get('mp_per_level', 0), key="mp_per_level_input")
            physical_attack_per_level = st.number_input("Physical Attack per Level", value=current_record.get('physical_attack_per_level', 0), key="physical_attack_per_level_input")
        with col2:
            physical_defense_per_level = st.number_input("Physical Defense per Level", value=current_record.get('physical_defense_per_level', 0), key="physical_defense_per_level_input")
            agility_per_level = st.number_input("Agility per Level", value=current_record.get('agility_per_level', 0), key="agility_per_level_input")
            magical_attack_per_level = st.number_input("Magical Attack per Level", value=current_record.get('magical_attack_per_level', 0), key="magical_attack_per_level_input")
        with col3:
            magical_defense_per_level = st.number_input("Magical Defense per Level", value=current_record.get('magical_defense_per_level', 0), key="magical_defense_per_level_input")
            resistance_per_level = st.number_input("Resistance per Level", value=current_record.get('resistance_per_level', 0), key="resistance_per_level_input")
            special_per_level = st.number_input("Special per Level", value=current_record.get('special_per_level', 0), key="special_per_level_input")

        ### Prerequisites
        with st.expander("Prerequisites"):
            if 'class_prerequisites' not in st.session_state:
                st.session_state.class_prerequisites = []
            if current_record and 'id' in current_record:
                prereq_query = "SELECT * FROM class_prerequisites WHERE class_id = ?"
                with get_db_connection() as conn:
                    prereq_df = pd.read_sql_query(prereq_query, conn, params=[current_record['id']])
                    st.session_state.class_prerequisites = prereq_df.to_dict('records')
            for i, prereq in enumerate(st.session_state.class_prerequisites):
                st.write(f"Prerequisite {i+1}")
                col1, col2, col3, col4 = st.columns(4)
                with col1:
                    group = st.number_input("Group", min_value=1, value=prereq.get('prerequisite_group', 1), key=f"prereq_group_{i}")
                with col2:
                    prereq_type = st.text_input("Type", value=prereq.get('prerequisite_type', ''), key=f"prereq_type_{i}")
                with col3:
                    target_id = st.number_input("Target ID", value=prereq.get('target_id', 0), key=f"prereq_target_id_{i}")
                with col4:
                    req_level = st.number_input("Required Level", min_value=0, value=prereq.get('required_level', 0), key=f"prereq_level_{i}")
                col5, col6, col7 = st.columns(3)
                with col5:
                    min_val = st.number_input("Min Value", value=prereq.get('min_value', 0), key=f"prereq_min_{i}")
                with col6:
                    max_val = st.number_input("Max Value", value=prereq.get('max_value', 0), key=f"prereq_max_{i}")
                with col7:
                    if st.button("Remove", key=f"remove_prereq_{i}"):
                        st.session_state.class_prerequisites.pop(i)
                        st.rerun()
                st.session_state.class_prerequisites[i].update({
                    'prerequisite_group': group,
                    'prerequisite_type': prereq_type,
                    'target_id': target_id,
                    'required_level': req_level,
                    'min_value': min_val,
                    'max_value': max_val
                })
            if st.button("Add Prerequisite"):
                st.session_state.class_prerequisites.append({
                    'prerequisite_group': 1, 'prerequisite_type': '', 'target_id': 0,
                    'required_level': 0, 'min_value': 0, 'max_value': 0
                })
                st.rerun()

        ### Exclusions
        with st.expander("Exclusions"):
            if 'class_exclusions' not in st.session_state:
                st.session_state.class_exclusions = []
            if current_record and 'id' in current_record:
                excl_query = "SELECT * FROM class_exclusions WHERE class_id = ?"
                with get_db_connection() as conn:
                    excl_df = pd.read_sql_query(excl_query, conn, params=[current_record['id']])
                    st.session_state.class_exclusions = excl_df.to_dict('records')
            for i, excl in enumerate(st.session_state.class_exclusions):
                st.write(f"Exclusion {i+1}")
                col1, col2, col3, col4 = st.columns(4)
                with col1:
                    excl_type = st.text_input("Type", value=excl.get('exclusion_type', ''), key=f"excl_type_{i}")
                with col2:
                    target_id = st.number_input("Target ID", value=excl.get('target_id', 0), key=f"excl_target_id_{i}")
                with col3:
                    min_val = st.number_input("Min Value", value=excl.get('min_value', 0), key=f"excl_min_{i}")
                with col4:
                    max_val = st.number_input("Max Value", value=excl.get('max_value', 0), key=f"excl_max_{i}")
                col5 = st.columns(1)[0]
                with col5:
                    if st.button("Remove", key=f"remove_excl_{i}"):
                        st.session_state.class_exclusions.pop(i)
                        st.rerun()
                st.session_state.class_exclusions[i].update({
                    'exclusion_type': excl_type,
                    'target_id': target_id,
                    'min_value': min_val,
                    'max_value': max_val
                })
            if st.button("Add Exclusion"):
                st.session_state.class_exclusions.append({
                    'exclusion_type': '', 'target_id': 0, 'min_value': 0, 'max_value': 0
                })
                st.rerun()

        ### Equipment Slots (Placeholder for Race Classes)
        if is_racial:
            with st.expander("Equipment Slots (Placeholder)"):
                st.write("TODO: Implement equipment slot editor for defining slots on a race-by-race basis.")

        ### Action Buttons
        col1, col2, col3 = st.columns(3)
        with col1:
            submit_button = st.form_submit_button("Create Record" if st.session_state.current_class_id == 0 else "Save Record")
        with col2:
            copy_button = st.form_submit_button("Copy Record")
        with col3:
            delete_button = st.form_submit_button("Delete Record", disabled=st.session_state.current_class_id == 0)

    ### Form Submission Handling
    if submit_button:
        record_data = {
            'id': st.session_state.current_class_id,
            'name': name,
            'description': description,
            'class_type': class_type,
            'is_racial': is_racial,
            'category_id': category,
            'subcategory_id': subcategory,
            'base_hp': base_hp,
            'base_mp': base_mp,
            'base_physical_attack': base_physical_attack,
            'base_physical_defense': base_physical_defense,
            'base_agility': base_agility,
            'base_magical_attack': base_magical_attack,
            'base_magical_defense': base_magical_defense,
            'base_resistance': base_resistance,
            'base_special': base_special,
            'hp_per_level': hp_per_level,
            'mp_per_level': mp_per_level,
            'physical_attack_per_level': physical_attack_per_level,
            'physical_defense_per_level': physical_defense_per_level,
            'agility_per_level': agility_per_level,
            'magical_attack_per_level': magical_attack_per_level,
            'magical_defense_per_level': magical_defense_per_level,
            'resistance_per_level': resistance_per_level,
            'special_per_level': special_per_level
        }
        is_new = st.session_state.current_class_id == 0
        if save_class_record(record_data, is_new):
            class_id = record_data['id']
            # Save Prerequisites
            if not is_new:
                with get_db_connection() as conn:
                    conn.execute("DELETE FROM class_prerequisites WHERE class_id = ?", [class_id])
            for prereq in st.session_state.class_prerequisites:
                query = """
                    INSERT INTO class_prerequisites (class_id, prerequisite_group, prerequisite_type, target_id, required_level, min_value, max_value)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                """
                with get_db_connection() as conn:
                    conn.execute(query, [class_id, prereq['prerequisite_group'], prereq['prerequisite_type'], prereq['target_id'],
                                        prereq['required_level'], prereq['min_value'], prereq['max_value']])
                    conn.commit()
            # Save Exclusions
            if not is_new:
                with get_db_connection() as conn:
                    conn.execute("DELETE FROM class_exclusions WHERE class_id = ?", [class_id])
            for excl in st.session_state.class_exclusions:
                query = "INSERT INTO class_exclusions (class_id, exclusion_type, target_id, min_value, max_value) VALUES (?, ?, ?, ?, ?)"
                with get_db_connection() as conn:
                    conn.execute(query, [class_id, excl['exclusion_type'], excl['target_id'], excl['min_value'], excl['max_value']])
                    conn.commit()
            st.success("Class and associated data saved successfully!")

    elif copy_button:
        st.session_state.current_class_id = 0
        st.rerun()

    elif delete_button:
        if delete_class_record(st.session_state.current_class_id):
            st.success("Record deleted successfully!")
            st.session_state.current_class_id = 0
            st.rerun()

    ### Spell List Navigation
    if st.session_state.current_class_id != 0:
        if st.button("Edit Spell List"):
            st.session_state.view = "spells"
            st.rerun()