# ./FrontEnd/CharacterInfo/views/JobClasses/interface.py

import streamlit as st
import pandas as pd
from typing import Dict, List, Tuple
from .database import (
    get_db_connection,
    get_class_details,
    get_class_prerequisites,
    get_class_exclusions,
    save_class,
    delete_class,
    copy_class
)
from .forms import (
    render_basic_info,
    render_stats_tab,
    render_prerequisite_tab,
    render_exclusion_tab
)

DEFAULT_QUERY = """
SELECT 
    c.id,
    c.name,
    c.description,
    ct.name as type,
    cc.name as category,
    cs.name as subcategory
FROM classes c 
JOIN class_types ct ON c.class_type = ct.id
JOIN class_categories cc ON c.category_id = cc.id
LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
WHERE c.is_racial = 0
ORDER BY cc.name, c.name;
"""

def execute_query(query: str) -> Tuple[List[Dict], List[str]]:
    """Execute SQL query and return results with column names"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute(query)
        columns = [description[0] for description in cursor.description]
        results = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return results, columns
    finally:
        conn.close()

def render_job_classes_interface():
    """Main interface for job classes management"""
    st.header("Job Classes Editor")
    
    # Initialize session state
    if 'sql_query' not in st.session_state:
        st.session_state.sql_query = DEFAULT_QUERY
    if 'selected_class_id' not in st.session_state:
        st.session_state.selected_class_id = None
    
    # SQL Query input
    st.session_state.sql_query = st.text_area(
        "SQL Query", 
        value=st.session_state.sql_query,
        height=100
    )
    
    try:
        # Execute query and display results
        results, columns = execute_query(st.session_state.sql_query)
        
        # Create DataFrame for display
        df_data = []
        for idx, row in enumerate(results):
            row_data = {'select': False}
            for col in columns:
                val = row.get(col, '')
                if isinstance(val, str) and len(val) > 50:
                    val = val[:47] + "..."
                row_data[col] = val
            df_data.append(row_data)

        # Create DataFrame
        df = pd.DataFrame(df_data)

        # Display table with selection
        selection = st.data_editor(
            df,
            column_config={
                "select": st.column_config.CheckboxColumn(
                    "Select",
                    default=False,
                    help="Select a class to edit"
                )
            },
            disabled=[col for col in df.columns if col != 'select'],
            hide_index=True
        )

        # Handle selection
        if selection is not None and 'select' in selection:
            selected_rows = selection[selection['select']].index
            if len(selected_rows) > 0:
                selected_idx = selected_rows[0]
                st.session_state.selected_class_id = results[selected_idx]['id']
                # Deselect all other rows
                for idx in selected_rows[1:]:
                    selection.at[idx, 'select'] = False
            else:
                st.session_state.selected_class_id = None
        
        # Copy/Delete buttons
        col1, col2 = st.columns(2)
        with col1:
            if st.button("Copy Selected Class", use_container_width=True):
                if st.session_state.selected_class_id:
                    success, message, new_class_id = copy_class(st.session_state.selected_class_id)
                    if success:
                        st.success(message)
                        st.session_state.selected_class_id = new_class_id
                        st.rerun()
                    else:
                        st.error(message)
                else:
                    st.error("Please select a class to copy")
        
        with col2:
            if st.button("Delete Selected Class", use_container_width=True):
                if st.session_state.selected_class_id:
                    success, message = delete_class(st.session_state.selected_class_id)
                    if success:
                        st.success(message)
                        st.session_state.selected_class_id = None
                        st.rerun()
                    else:
                        st.error(message)

        # Display class editing form if a class is selected
        st.markdown("---")
        if st.session_state.selected_class_id is not None:
            class_data = get_class_details(st.session_state.selected_class_id)
            
            # Initialize prerequisite and exclusion state
            if class_data:
                if 'prereq_groups' not in st.session_state:
                    prereqs = get_class_prerequisites(st.session_state.selected_class_id)
                    if prereqs:
                        current_group = []
                        current_group_num = prereqs[0]['prerequisite_group']
                        st.session_state.prereq_groups = []
                        for prereq in prereqs:
                            if prereq['prerequisite_group'] != current_group_num:
                                if current_group:
                                    st.session_state.prereq_groups.append(current_group)
                                current_group = []
                                current_group_num = prereq['prerequisite_group']
                            current_group.append(prereq)
                        if current_group:
                            st.session_state.prereq_groups.append(current_group)
                
                if 'exclusions' not in st.session_state:
                    exclusions = get_class_exclusions(st.session_state.selected_class_id)
                    if exclusions:
                        st.session_state.exclusions = exclusions

            with st.form("class_details_form"):
                # Basic info
                basic_info = render_basic_info(class_data)
                
                # Stats/Prerequisites/Exclusions tabs
                stats_tab, prereq_tab, excl_tab = st.tabs(["Stats", "Prerequisites", "Exclusions"])
                
                with stats_tab:
                    stats_data = render_stats_tab(class_data)
                
                with prereq_tab:
                    prereq_data = render_prerequisite_tab()
                
                with excl_tab:
                    excl_data = render_exclusion_tab()

                # Save button
                if st.form_submit_button("Save All Changes", use_container_width=True):
                    # Start transaction
                    conn = get_db_connection()
                    cursor = conn.cursor()
                    try:
                        cursor.execute("BEGIN TRANSACTION")

                        # Prepare class data
                        updated_class_data = {
                            'id': st.session_state.selected_class_id,
                            **basic_info,
                            **stats_data
                        }
                        
                        # Save main class data
                        success, message = save_class(updated_class_data)
                        if not success:
                            raise Exception(message)

                        # Delete existing prerequisites and exclusions
                        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", 
                                     (st.session_state.selected_class_id,))
                        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", 
                                     (st.session_state.selected_class_id,))

                        # Save prerequisites
                        for group_idx, prereq_group in enumerate(prereq_data):
                            for prereq in prereq_group:
                                cursor.execute("""
                                    INSERT INTO class_prerequisites (
                                        class_id, prerequisite_group, prerequisite_type,
                                        target_id, required_level, min_value, max_value
                                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                                """, (
                                    st.session_state.selected_class_id,
                                    group_idx,
                                    prereq['type'],
                                    prereq.get('target_id'),
                                    prereq.get('required_level'),
                                    prereq.get('min_value'),
                                    prereq.get('max_value')
                                ))

                        # Save exclusions
                        for excl in excl_data:
                            cursor.execute("""
                                INSERT INTO class_exclusions (
                                    class_id, exclusion_type, target_id,
                                    min_value, max_value
                                ) VALUES (?, ?, ?, ?, ?)
                            """, (
                                st.session_state.selected_class_id,
                                excl['type'],
                                excl.get('target_id'),
                                excl.get('min_value'),
                                excl.get('max_value')
                            ))

                        cursor.execute("COMMIT")
                        st.success("Class details, prerequisites, and exclusions saved successfully!")
                        # Clear session state
                        if 'prereq_groups' in st.session_state:
                            del st.session_state.prereq_groups
                        if 'exclusions' in st.session_state:
                            del st.session_state.exclusions
                        st.rerun()

                    except Exception as e:
                        cursor.execute("ROLLBACK")
                        st.error(f"Error saving changes: {str(e)}")
                    finally:
                        conn.close()
                        
    except Exception as e:
        st.error(f"Error executing query: {str(e)}")