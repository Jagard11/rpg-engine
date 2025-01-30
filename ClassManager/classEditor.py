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

def get_foreign_key_options(table_name: str) -> Dict[int, str]:
    """Get options for foreign key dropdown menus"""
    query = f"SELECT id, name FROM {table_name}"
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn)
            return dict(zip(df['id'], df['name']))
    except Exception as e:
        st.error(f"Error loading {table_name}: {e}")
        return {}

def load_class_record(class_id: int) -> Optional[Dict[str, Any]]:
    """Load a specific class record"""
    if class_id == 0:
        return None
        
    query = """
    SELECT * FROM classes WHERE id = ?
    """
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[class_id])
            if not df.empty:
                return df.iloc[0].to_dict()
    except Exception as e:
        st.error(f"Error loading class record: {e}")
    return None

def save_class_record(record_data: Dict[str, Any], is_new: bool = True) -> bool:
    """Save the class record to database"""
    if is_new:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        placeholders = ','.join(['?' for _ in columns])
        query = f"""
        INSERT INTO classes ({','.join(columns)})
        VALUES ({placeholders})
        """
        values = [record_data[col] for col in columns]
    else:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        set_clause = ','.join([f"{col} = ?" for col in columns])
        query = f"""
        UPDATE classes 
        SET {set_clause}, updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
        """
        values = [record_data[col] for col in columns] + [record_data['id']]
    
    try:
        with get_db_connection() as conn:
            conn.execute(query, values)
            conn.commit()
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
    if 'selected_class_name' in st.session_state and 'class_query_results' in st.session_state:
        selected_name = st.session_state.selected_class_name
        if selected_name:
            selected_record = st.session_state.class_query_results[
                st.session_state.class_query_results['name'] == selected_name
            ]
            if not selected_record.empty:
                st.session_state.current_class_id = int(selected_record.iloc[0]['id'])
                current_record = load_class_record(st.session_state.current_class_id)
            else:
                current_record = {}
        else:
            current_record = {}
    else:
        # Load current record data if already set
        current_record = load_class_record(st.session_state.current_class_id) or {}
    
    # Form for class data
    with st.form("class_editor_form", clear_on_submit=False):
        # ID field (non-editable)
        st.number_input("ID", value=st.session_state.current_class_id, disabled=True, key="class_id_input")
        
        # Basic information
        name = st.text_input("Name", value=current_record.get('name', ''), key="class_name_input")
        description = st.text_area("Description", value=current_record.get('description', ''), key="class_description_input")
        
        # Foreign key selections
        current_class_type = current_record.get('class_type', list(class_types.keys())[0] if class_types else None)
        class_type = st.selectbox(
            "Class Type",
            options=list(class_types.keys()),
            format_func=lambda x: class_types.get(x, ''),
            key="class_type_input",
            index=list(class_types.keys()).index(current_class_type) if current_class_type in class_types else 0
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
            index=list(subcategories.keys()).index(current_subcategory) if current_subcategory in subcategories else 0
        )
        
        # Boolean fields
        is_racial = st.checkbox("Is Racial Class", value=current_record.get('is_racial', False), key="is_racial_input")
        
        # Base stats
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
            
        # Per level stats
        st.subheader("Stats Per Level")
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
        
        # Action buttons
        col1, col2, col3 = st.columns(3)
        with col1:
            submit_button = st.form_submit_button(
                "Create Record" if st.session_state.current_class_id == 0 else "Save Record",
                use_container_width=True
            )
        with col2:
            copy_button = st.form_submit_button(
                "Copy Record",
                use_container_width=True
            )
        with col3:
            delete_button = st.form_submit_button(
                "Delete Record",
                use_container_width=True,
                disabled=st.session_state.current_class_id == 0
            )
    
    # Handle form submissions
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
        
        if save_class_record(record_data, st.session_state.current_class_id == 0):
            st.success("Record saved successfully!")
            
    elif copy_button:
        st.session_state.current_class_id = 0
        st.rerun()
        
    elif delete_button:
        if delete_class_record(st.session_state.current_class_id):
            st.success("Record deleted successfully!")
            st.session_state.current_class_id = 0
            st.rerun()