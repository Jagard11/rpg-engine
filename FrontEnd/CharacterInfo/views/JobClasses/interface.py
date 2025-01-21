# ./FrontEnd/CharacterInfo/views/JobClasses/interface.py

import streamlit as st
from typing import Dict, List, Optional
from .database import (
    get_db_connection,
    get_all_job_classes,
    get_class_details,
    copy_class,
    delete_class
)
from .editor import render_class_editor

def render_job_classes_interface(browse_mode: bool = False):
    """Render the job classes management interface
    
    Args:
        browse_mode (bool): If True, show read-only browsing interface. If False, show editing interface.
    """
    # Initialize session state with mode-specific keys
    mode_prefix = "browse_" if browse_mode else "edit_"
    
    if f'{mode_prefix}selected_job_class_id' not in st.session_state:
        st.session_state[f'{mode_prefix}selected_job_class_id'] = None
    if f'{mode_prefix}filter_category' not in st.session_state:
        st.session_state[f'{mode_prefix}filter_category'] = "All"
    if f'{mode_prefix}filter_type' not in st.session_state:
        st.session_state[f'{mode_prefix}filter_type'] = "All"
    if f'{mode_prefix}show_prerequisites' not in st.session_state:
        st.session_state[f'{mode_prefix}show_prerequisites'] = False

    # Filters in columns
    st.subheader("Filters")
    col1, col2, col3 = st.columns(3)
    
    with col1:
        # Category filter
        categories = ["All"] + get_categories()
        st.session_state[f'{mode_prefix}filter_category'] = st.selectbox(
            "Category",
            options=categories,
            index=categories.index(st.session_state[f'{mode_prefix}filter_category']),
            key=f"{mode_prefix}category_select"
        )

    with col2:
        # Class type filter
        types = ["All"] + get_class_types()
        st.session_state[f'{mode_prefix}filter_type'] = st.selectbox(
            "Class Type",
            options=types,
            index=types.index(st.session_state[f'{mode_prefix}filter_type']),
            key=f"{mode_prefix}type_select"
        )

    with col3:
        # Prerequisites filter
        st.session_state[f'{mode_prefix}show_prerequisites'] = st.checkbox(
            "Show Prerequisites",
            value=st.session_state[f'{mode_prefix}show_prerequisites'],
            key=f"{mode_prefix}prereq_check"
        )

    # Get filtered classes
    classes = get_filtered_classes(
        category=st.session_state[f'{mode_prefix}filter_category'],
        class_type=st.session_state[f'{mode_prefix}filter_type'],
        show_prerequisites=st.session_state[f'{mode_prefix}show_prerequisites']
    )

    # Class selection section
    st.markdown("---")
    col1, col2 = st.columns([2, 1])
    
    with col1:
        st.write(f"Found {len(classes)} classes")
        
        if classes:
            options = [f"{c['name']} ({c['category']} - {c['type']})" for c in classes]
            selected_index = st.selectbox(
                "Select Class",
                range(len(options)),
                format_func=lambda x: options[x],
                key=f"{mode_prefix}class_select"
            )
            
            if selected_index is not None:
                st.session_state[f'{mode_prefix}selected_job_class_id'] = classes[selected_index]['id']

    # Only show edit/delete buttons in edit mode
    if not browse_mode:
        with col2:
            if st.session_state[f'{mode_prefix}selected_job_class_id']:
                if st.button("Copy Selected Class", key=f"{mode_prefix}copy_btn", use_container_width=True):
                    success, message, new_id = copy_class(st.session_state[f'{mode_prefix}selected_job_class_id'])
                    if success:
                        st.success(message)
                        st.session_state[f'{mode_prefix}selected_job_class_id'] = new_id
                        st.rerun()
                    else:
                        st.error(message)

                if st.button("Delete Selected Class", key=f"{mode_prefix}delete_btn", use_container_width=True):
                    success, message = delete_class(st.session_state[f'{mode_prefix}selected_job_class_id'])
                    if success:
                        st.success(message)
                        st.session_state[f'{mode_prefix}selected_job_class_id'] = None
                        st.rerun()
                    else:
                        st.error(message)

    # Show class details if selected
    if st.session_state[f'{mode_prefix}selected_job_class_id']:
        st.markdown("---")
        class_data = get_class_details(st.session_state[f'{mode_prefix}selected_job_class_id'])
        if class_data:
            if browse_mode:
                render_class_details(class_data)
            else:
                render_class_editor(class_data)
    else:
        st.markdown("---")
        if browse_mode:
            st.info("Select a class to view its details.")
        else:
            st.info("Select a class to edit or create a new one.")

def render_class_details(class_data: Dict):
    """Render read-only class details for browse mode"""
    st.header(class_data['name'])
    
    col1, col2 = st.columns(2)
    with col1:
        st.write("**Category:**", class_data['category_name'])
        st.write("**Type:**", class_data['type_name'])
        if class_data['subcategory_name']:
            st.write("**Subcategory:**", class_data['subcategory_name'])
    
    with col2:
        st.write("**Base Stats:**")
        st.write(f"HP: {class_data['base_hp']} (+{class_data['hp_per_level']}/level)")
        st.write(f"MP: {class_data['base_mp']} (+{class_data['mp_per_level']}/level)")
        st.write(f"Physical Attack: {class_data['base_physical_attack']} (+{class_data['physical_attack_per_level']}/level)")
        st.write(f"Physical Defense: {class_data['base_physical_defense']} (+{class_data['physical_defense_per_level']}/level)")
        st.write(f"Agility: {class_data['base_agility']} (+{class_data['agility_per_level']}/level)")
    
    if class_data['description']:
        st.markdown("### Description")
        st.write(class_data['description'])

def get_categories() -> List[str]:
    """Get list of non-racial class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT DISTINCT cc.name
            FROM class_categories cc
            JOIN classes c ON cc.id = c.category_id
            WHERE c.is_racial = 0
            ORDER BY cc.name
        """)
        return [row[0] for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_types() -> List[str]:
    """Get list of class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT DISTINCT ct.name
            FROM class_types ct
            JOIN classes c ON ct.id = c.class_type
            WHERE c.is_racial = 0
            ORDER BY ct.name
        """)
        return [row[0] for row in cursor.fetchall()]
    finally:
        conn.close()

def get_filtered_classes(
    category: str = "All",
    class_type: str = "All",
    show_prerequisites: bool = False
) -> List[Dict]:
    """Get filtered list of job classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Build query conditions
        conditions = ["c.is_racial = 0"]
        params = []

        if category != "All":
            conditions.append("cc.name = ?")
            params.append(category)

        if class_type != "All":
            conditions.append("ct.name = ?")
            params.append(class_type)

        if show_prerequisites:
            conditions.append("""
                EXISTS (
                    SELECT 1 FROM class_prerequisites cp 
                    WHERE cp.class_id = c.id
                )
            """)

        # Construct and execute query
        query = f"""
            SELECT 
                c.id,
                c.name,
                cc.name as category,
                ct.name as type
            FROM classes c
            JOIN class_categories cc ON c.category_id = cc.id
            JOIN class_types ct ON c.class_type = ct.id
            WHERE {" AND ".join(conditions)}
            ORDER BY cc.name, c.name
        """
        
        cursor.execute(query, params)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()