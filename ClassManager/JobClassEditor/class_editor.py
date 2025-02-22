# ./ClassManager/JobClassEditor/class_editor.py

import streamlit as st
import pandas as pd
from typing import Optional, Dict, Any
from datetime import datetime
from .utils import get_db_connection, get_foreign_key_options
from .basic_info_tab import render_basic_info_tab
from .stats_tab import render_stats_tab
from .prerequisites_tab import render_prerequisites_tab
from .exclusions_tab import render_exclusions_tab
from .conditions_tab import render_conditions_tab
from .spell_list_tab import render_spell_list_tab

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
    """Render the class editor interface with subtabs"""
    parent_container = st.container()
    
    with parent_container:
        st.header("Job Class Editor")

        # Load foreign key options
        class_types = get_foreign_key_options('class_types')
        categories = get_foreign_key_options('class_categories')
        subcategories = get_foreign_key_options('class_subcategories')

        # Initialize or get current record ID
        if 'current_class_id' not in st.session_state:
            st.session_state.current_class_id = 0

        # Check for edit_id in query parameters
        query_params = st.query_params
        if 'edit_id' in query_params and query_params.get('script') == 'job_class_editor' and query_params.get('mode') == 'edit':
            try:
                edit_id = int(query_params['edit_id'])
                st.session_state.current_class_id = edit_id
            except ValueError:
                st.error("Invalid edit_id provided")

        # Handle "create" mode
        if query_params.get('script') == 'job_class_editor' and query_params.get('mode') == 'create':
            st.session_state.current_class_id = 0

        # Load record
        current_record = load_class_record(st.session_state.current_class_id) or {}

        # Initialize session state for prerequisites and exclusions
        if 'class_prerequisites' not in st.session_state:
            st.session_state.class_prerequisites = []
        if 'class_exclusions' not in st.session_state:
            st.session_state.class_exclusions = []
        if current_record and 'id' in current_record:
            with get_db_connection() as conn:
                prereq_df = pd.read_sql_query("SELECT * FROM class_prerequisites WHERE class_id = ?", conn, params=[current_record['id']])
                st.session_state.class_prerequisites = prereq_df.to_dict('records')
                excl_df = pd.read_sql_query("SELECT * FROM class_exclusions WHERE class_id = ?", conn, params=[current_record['id']])
                st.session_state.class_exclusions = excl_df.to_dict('records')

        with st.form("class_editor_form", clear_on_submit=False):
            # Define tabs (removed "Equipment Slots")
            tab1, tab2, tab3, tab4, tab5, tab6 = st.tabs([
                "Basic Info", "Stats", "Prerequisites", "Exclusions", 
                "Conditions", "Spell List"
            ])

            # Render each tab using modular functions
            record_data = {}
            with tab1:
                record_data.update(render_basic_info_tab(current_record, class_types, categories, subcategories))
            with tab2:
                record_data.update(render_stats_tab(current_record))
            with tab3:
                render_prerequisites_tab()
            with tab4:
                render_exclusions_tab()
            with tab5:
                render_conditions_tab(current_record)
            with tab6:
                render_spell_list_tab(current_record)

            # Actions buttons at the bottom of the form
            st.subheader("Actions")
            col1, col2, col3 = st.columns(3)
            with col1:
                submit_button = st.form_submit_button("Create Record" if st.session_state.current_class_id == 0 else "Save Record")
            with col2:
                copy_button = st.form_submit_button("Copy Record")
            with col3:
                delete_button = st.form_submit_button("Delete Record", disabled=st.session_state.current_class_id == 0)

        # Form Submission Handling
        if submit_button:
            record_data['id'] = st.session_state.current_class_id
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
                st.rerun()

        elif copy_button:
            st.session_state.current_class_id = 0
            st.rerun()

        elif delete_button:
            if delete_class_record(st.session_state.current_class_id):
                st.success("Record deleted successfully!")
                st.session_state.current_class_id = 0
                st.rerun()

if __name__ == "__main__":
    render_class_editor()