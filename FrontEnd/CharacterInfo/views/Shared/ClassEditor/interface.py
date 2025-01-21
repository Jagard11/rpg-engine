# ./FrontEnd/CharacterInfo/views/Shared/ClassEditor/interface.py

import streamlit as st
from typing import Dict, List, Optional, Tuple, Callable
from .forms import (
    render_basic_info,
    render_stats_section,
    render_prerequisites_section,
    render_exclusions_section
)
from ...JobClasses.database import (
    get_class_details,
    get_class_prerequisites,
    get_class_exclusions,
    save_class
)

def handle_prerequisite_group_add():
    """Add a new prerequisite group"""
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    st.session_state.prereq_groups.append([])

def handle_prerequisite_add(group_idx: int, prereq_type: str):
    """Add a new prerequisite to a group"""
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    st.session_state.prereq_groups[group_idx].append({
        'type': prereq_type,
        'target_id': None,
        'required_level': None,
        'min_value': None,
        'max_value': None
    })

def handle_prerequisite_remove(group_idx: int, req_idx: int):
    """Remove a prerequisite from a group"""
    st.session_state.prereq_groups[group_idx].pop(req_idx)

def handle_prerequisite_group_remove(group_idx: int):
    """Remove a prerequisite group"""
    st.session_state.prereq_groups.pop(group_idx)

def handle_exclusion_add(exclusion_type: str):
    """Add a new exclusion"""
    if 'exclusions' not in st.session_state:
        st.session_state.exclusions = []
    st.session_state.exclusions.append({
        'type': exclusion_type,
        'target_id': None,
        'min_value': None,
        'max_value': None
    })

def handle_exclusion_remove(idx: int):
    """Remove an exclusion"""
    st.session_state.exclusions.pop(idx)

def render_class_editor(
    class_id: Optional[int] = None,
    is_racial: bool = False,
    extra_fields: Optional[List[Tuple[str, Callable]]] = None,
    mode_prefix: str = ""
):
    """Render the class editor interface"""
    # Initialize session state
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    if 'exclusions' not in st.session_state:
        st.session_state.exclusions = []
    
    # Load class data if editing
    class_data = None
    if class_id:
        class_data = get_class_details(class_id)
        
        # Load prerequisites and exclusions on first load
        if class_data and not st.session_state.prereq_groups:
            prereqs = get_class_prerequisites(class_id)
            if prereqs:
                # Group prerequisites
                current_group = []
                current_group_num = prereqs[0]['prerequisite_group']
                for prereq in prereqs:
                    if prereq['prerequisite_group'] != current_group_num:
                        if current_group:
                            st.session_state.prereq_groups.append(current_group)
                        current_group = []
                        current_group_num = prereq['prerequisite_group']
                    current_group.append(prereq)
                if current_group:
                    st.session_state.prereq_groups.append(current_group)
            
        if class_data and not st.session_state.exclusions:
            exclusions = get_class_exclusions(class_id)
            if exclusions:
                st.session_state.exclusions = exclusions

    # Action buttons outside forms
    if st.button(f"Add Prerequisite Group (OR)", key=f"{mode_prefix}add_prereq_group"):
        handle_prerequisite_group_add()
        st.experimental_rerun()

    # Render forms and manage state
    with st.form(f"{mode_prefix}class_editor_form", clear_on_submit=False):
        basic_info = render_basic_info(
            class_data or {},
            is_racial=is_racial,
            extra_fields=extra_fields
        )
        
        # Stats/Prerequisites/Exclusions tabs
        stats_tab, prereq_tab, excl_tab = st.tabs(["Stats", "Prerequisites", "Exclusions"])
        
        with stats_tab:
            stats_data = render_stats_section(class_data or {})
        
        with prereq_tab:
            prereqs = render_prerequisites_section(
                mode_prefix=mode_prefix,
                include_racial=is_racial
            )
        
        with excl_tab:
            exclusions = render_exclusions_section(
                mode_prefix=mode_prefix,
                include_racial=is_racial
            )

        # Save submit button
        if st.form_submit_button("Save Changes"):
            try:
                # Combine all data
                updated_data = {
                    'id': class_id,
                    **basic_info,
                    **stats_data
                }
                
                # Save everything
                success, message = save_class(
                    updated_data,
                    prerequisites=prereqs,
                    exclusions=exclusions
                )
                
                if success:
                    st.success(message)
                    # Clear session state
                    if 'prereq_groups' in st.session_state:
                        del st.session_state.prereq_groups
                    if 'exclusions' in st.session_state:
                        del st.session_state.exclusions
                    st.experimental_rerun()
                else:
                    st.error(message)
            
            except Exception as e:
                st.error(f"Error saving changes: {str(e)}")

    # Action buttons for prerequisites after form
    for group_idx, prereq_group in enumerate(st.session_state.prereq_groups):
        col1, col2 = st.columns([3, 1])
        with col1:
            st.write(f"Group {group_idx + 1}")
        with col2:
            if st.button(f"Add to Group {group_idx + 1}", key=f"{mode_prefix}add_req_{group_idx}"):
                prereq_type = st.session_state.get(f"{mode_prefix}prereq_type_{group_idx}")
                if prereq_type:
                    handle_prerequisite_add(group_idx, prereq_type)
                    st.experimental_rerun()

        for req_idx, _ in enumerate(prereq_group):
            if st.button(f"Remove {req_idx + 1}", key=f"{mode_prefix}remove_req_{group_idx}_{req_idx}"):
                handle_prerequisite_remove(group_idx, req_idx)
                st.experimental_rerun()

        if st.button(f"Remove Group {group_idx + 1}", key=f"{mode_prefix}remove_group_{group_idx}"):
            handle_prerequisite_group_remove(group_idx)
            st.experimental_rerun()

    # Action buttons for exclusions after form
    for idx, _ in enumerate(st.session_state.exclusions):
        if st.button(f"Remove Exclusion {idx + 1}", key=f"{mode_prefix}remove_excl_{idx}"):
            handle_exclusion_remove(idx)
            st.experimental_rerun()

    if st.button("Add Exclusion", key=f"{mode_prefix}add_exclusion"):
        exclusion_type = st.session_state.get(f"{mode_prefix}excl_type")
        if exclusion_type:
            handle_exclusion_add(exclusion_type)
            st.experimental_rerun()