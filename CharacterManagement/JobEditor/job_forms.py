# ./CharacterManagement/JobEditor/forms.py

import streamlit as st
from typing import Dict, Optional
from .database import (
    get_class_types,
    get_class_categories,
    save_job_class
)

def render_job_class_form(selected_class: Optional[Dict] = None, mode: str = "create", record_id: Optional[int] = None) -> None:
    """Render the job class editing form with dynamic subtitle
    
    Args:
        selected_class: Dictionary containing the selected class data (None for new)
        mode: "edit" or "create"
        record_id: ID of the record being edited (if mode=="edit")
    """
    with st.form("job_class_form"):
        # Set the subtitle based on mode and record ID
        if mode == "edit" and record_id:
            st.subheader(f"Edit Job Class - {record_id}")
        else:
            st.subheader("Create New Job Class")
        
        # Basic info
        col1, col2 = st.columns(2)
        with col1:
            name = st.text_input("Name", value=selected_class['name'] if selected_class else '')
            
        with col2:
            class_types = get_class_types()
            type_id = st.selectbox(
                "Type",
                options=[t["id"] for t in class_types],
                format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
                index=0
            )
        
        description = st.text_area("Description", value=selected_class['description'] if selected_class else '')
        
        # Stats
        st.subheader("Stats")
        col1, col2 = st.columns(2)
        
        with col1:
            base_hp = st.number_input("Base HP", value=selected_class['base_hp'] if selected_class else 0)
            base_mp = st.number_input("Base MP", value=selected_class['base_mp'] if selected_class else 0)
        
        with col2:
            hp_per_level = st.number_input("HP per Level", value=selected_class['hp_per_level'] if selected_class else 0)
            mp_per_level = st.number_input("MP per Level", value=selected_class['mp_per_level'] if selected_class else 0)
        
        # Save button
        if st.form_submit_button("Save Changes"):
            success, message = save_job_class({
                'id': selected_class['id'] if selected_class else None,
                'name': name,
                'description': description,
                'class_type': type_id,
                'base_hp': base_hp,
                'base_mp': base_mp,
                'hp_per_level': hp_per_level,
                'mp_per_level': mp_per_level
            })
            
            if success:
                st.success(message)
                st.rerun()
            else:
                st.error(message)

def render_filters():
    """Render the job class filters section"""
    st.subheader("Filters")
    col1, col2, col3 = st.columns(3)
    
    with col1:
        search = st.text_input("Search Classes", key="job_search")
    
    with col2:
        types = ["All"] + [t["name"] for t in get_class_types()]
        type_filter = st.selectbox("Class Type", types)
    
    with col3:
        categories = ["All"] + [c["name"] for c in get_class_categories()]
        category_filter = st.selectbox("Category", categories)
        
    return search, type_filter, category_filter

def render_class_selection(classes: list) -> Optional[Dict]:
    """Render the class selection section
    
    Args:
        classes: List of available classes
        
    Returns:
        Selected class data or None if no selection
    """
    if not classes:
        st.info("No classes found matching filters")
        return None
        
    st.write(f"Found {len(classes)} classes")
    
    # Class selection
    class_options = [f"{c['name']} ({c['category_name']} - {c['type_name']})" 
                    for c in classes]
    
    selected_idx = st.selectbox(
        "Select Class",
        range(len(class_options)),
        format_func=lambda x: class_options[x]
    )
    
    if selected_idx is not None:
        st.session_state.selected_job_id = classes[selected_idx]['id']
        return classes[selected_idx]
        
    return None

