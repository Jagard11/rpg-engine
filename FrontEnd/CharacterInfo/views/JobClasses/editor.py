# ./FrontEnd/CharacterInfo/views/JobClasses/editor.py

import streamlit as st
from typing import Dict, List, Optional
from .database import (
    get_class_types,
    get_class_categories,
    get_class_subcategories,
    get_all_classes,
    save_class
)

def render_class_editor(class_data: Dict, mode_prefix: str = "edit_"):
    """Render the job class editor interface with prerequisite and exclusion support
    
    Args:
        class_data (Dict): The class data to edit
        mode_prefix (str): Prefix to make keys unique between different modes
    """
    st.header("Job Class Editor")

    # Load prerequisites and exclusions if not in session state
    if 'prereq_groups' not in st.session_state and class_data:
        if class_data.get('prerequisites'):
            st.session_state.prereq_groups = class_data['prerequisites']
        else:
            st.session_state.prereq_groups = []

    if 'exclusions' not in st.session_state and class_data:
        if class_data.get('exclusions'):
            st.session_state.exclusions = class_data['exclusions']
        else:
            st.session_state.exclusions = []

    # Prerequisites and Exclusions sections (outside form)
    tab1, tab2 = st.tabs(["Prerequisites", "Exclusions"])
    
    with tab1:
        prereqs = render_prerequisites_section(mode_prefix)
    
    with tab2:
        exclusions = render_exclusions_section(mode_prefix)

    # Main form with basic info and stats
    with st.form("class_editor_form"):
        st.subheader("Basic Information")
        col1, col2 = st.columns(2)
        with col1:
            name = st.text_input("Name", value=class_data.get('name', ''))
        with col2:
            class_types = get_class_types()
            class_type = st.selectbox(
                "Class Type",
                options=[t['id'] for t in class_types],
                format_func=lambda x: next(t['name'] for t in class_types if t['id'] == x),
                index=next((i for i, t in enumerate(class_types) if t['id'] == class_data.get('class_type', 1)), 0)
            )

        description = st.text_area("Description", value=class_data.get('description', ''))

        st.markdown("---")
        st.subheader("Stats")
        stats_data = render_stats_section(class_data)

        # Save button
        if st.form_submit_button("Save Changes"):
            try:
                # Combine all data
                updated_data = {
                    'id': class_data.get('id'),
                    'name': name,
                    'description': description,
                    'class_type': class_type,
                    **stats_data
                }
                
                # Save everything
                success, message = save_class(
                    updated_data,
                    prerequisites=st.session_state.prereq_groups,
                    exclusions=st.session_state.exclusions
                )
                
                if success:
                    st.success(message)
                    st.rerun()
                else:
                    st.error(message)
            
            except Exception as e:
                st.error(f"Error saving changes: {str(e)}")

def render_prerequisites_section(mode_prefix: str = "") -> List[Dict]:
    """Render the prerequisites section with support for OR groups
    
    Args:
        mode_prefix (str): Prefix to make keys unique between different modes
    """
    st.subheader("Prerequisites")
    
    # Add new group button
    if st.button("Add Prerequisite Group (OR)", key=f"{mode_prefix}add_prereq_group"):
        st.session_state.prereq_groups.append([])
        st.rerun()
    
    # Render each group
    for group_idx, prereq_group in enumerate(st.session_state.prereq_groups):
        st.write(f"Group {group_idx + 1} (Any of these)")
        
        col1, col2 = st.columns([3, 1])
        with col1:
            prereq_type_mapping = {
                "Class": "specific_class",
                "Category Total": "category_total",
                "Subcategory Total": "subcategory_total",
                "Karma": "karma"
            }
            
            prereq_type_display = st.selectbox(
                "Type",
                list(prereq_type_mapping.keys()),
                key=f"{mode_prefix}prereq_type_{group_idx}"
            )
            prereq_type = prereq_type_mapping[prereq_type_display]
        
        with col2:
            if st.button("Add Requirement", key=f"{mode_prefix}add_req_group_{group_idx}"):
                st.session_state.prereq_groups[group_idx].append({
                    'type': prereq_type,
                    'target_id': None,
                    'required_level': None,
                    'min_value': None,
                    'max_value': None
                })
                st.rerun()
        
        # Render requirements in this group
        for req_idx, req in enumerate(prereq_group):
            st.markdown("---")
            cols = st.columns([3, 2, 1])
            
            with cols[0]:
                if req['type'] == "specific_class":
                    classes = get_all_classes()
                    options = [f"{c['name']} ({c['category']})" for c in classes]
                    selected = st.selectbox(
                        "Class",
                        range(len(options)),
                        format_func=lambda x: options[x],
                        key=f"{mode_prefix}class_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = classes[selected]['id']
                        level = st.number_input(
                            "Required Level",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"{mode_prefix}level_{group_idx}_{req_idx}"
                        )
                        req['required_level'] = level
                
                elif req['type'] in ["category_total", "subcategory_total"]:
                    items = get_class_categories() if req['type'] == "category_total" else get_class_subcategories()
                    options = [item['name'] for item in items]
                    selected = st.selectbox(
                        req['type'],
                        range(len(options)),
                        format_func=lambda x: options[x],
                        key=f"{mode_prefix}cat_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = items[selected]['id']
                        level = st.number_input(
                            "Required Total Levels",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"{mode_prefix}total_{group_idx}_{req_idx}"
                        )
                        req['required_level'] = level
                
                elif req['type'] == "karma":
                    min_val = st.number_input(
                        "Minimum Karma",
                        value=req.get('min_value', -1000),
                        key=f"{mode_prefix}karma_min_{group_idx}_{req_idx}"
                    )
                    max_val = st.number_input(
                        "Maximum Karma",
                        value=req.get('max_value', 1000),
                        key=f"{mode_prefix}karma_max_{group_idx}_{req_idx}"
                    )
                    req['min_value'] = min_val
                    req['max_value'] = max_val
            
            with cols[2]:
                if st.button("Remove", key=f"{mode_prefix}remove_req_{group_idx}_{req_idx}"):
                    st.session_state.prereq_groups[group_idx].pop(req_idx)
                    st.rerun()
        
        if st.button(f"Remove Group {group_idx + 1}", key=f"{mode_prefix}remove_group_{group_idx}"):
            st.session_state.prereq_groups.pop(group_idx)
            st.rerun()
    
    return st.session_state.prereq_groups

def render_exclusions_section(mode_prefix: str = "") -> List[Dict]:
    """Render the exclusions section
    
    Args:
        mode_prefix (str): Prefix to make keys unique between different modes
    """
    st.subheader("Exclusions")
    
    col1, col2 = st.columns([3, 1])
    with col1:
        exclusion_type_mapping = {
            "Class": "specific_class",
            "Category Total": "category_total",
            "Subcategory Total": "subcategory_total",
            "Karma": "karma"
        }
        
        exclusion_type_display = st.selectbox(
            "Type",
            list(exclusion_type_mapping.keys()),
            key="excl_type"
        )
        exclusion_type = exclusion_type_mapping[exclusion_type_display]
    
    with col2:
        if st.button("Add Exclusion", key="add_exclusion"):
            st.session_state.exclusions.append({
                'type': exclusion_type,
                'target_id': None,
                'min_value': None,
                'max_value': None
            })
            st.rerun()
    
    for idx, excl in enumerate(st.session_state.exclusions):
        st.markdown("---")
        cols = st.columns([3, 2, 1])
        
        with cols[0]:
            if excl['type'] == "specific_class":
                classes = get_all_classes()
                options = [f"{c['name']} ({c['category']})" for c in classes]
                selected = st.selectbox(
                    "Excluded Class",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"excl_class_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = classes[selected]['id']
            
            elif excl['type'] in ["category_total", "subcategory_total"]:
                items = get_class_categories() if excl['type'] == "category_total" else get_class_subcategories()
                options = [item['name'] for item in items]
                selected = st.selectbox(
                    f"Excluded {excl['type']}",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"excl_cat_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = items[selected]['id']
                    min_val = st.number_input(
                        "Minimum Levels",
                        value=excl.get('min_value', 1),
                        key=f"excl_min_{idx}"
                    )
                    excl['min_value'] = min_val
            
            elif excl['type'] == "karma":
                min_val = st.number_input(
                    "Min Karma",
                    value=excl.get('min_value', -1000),
                    key=f"excl_karma_min_{idx}"
                )
                max_val = st.number_input(
                    "Max Karma",
                    value=excl.get('max_value', 1000),
                    key=f"excl_karma_max_{idx}"
                )
                excl['min_value'] = min_val
                excl['max_value'] = max_val
        
        with cols[2]:
            if st.button("Remove", key=f"remove_excl_{idx}"):
                st.session_state.exclusions.pop(idx)
                st.rerun()
    
    return st.session_state.exclusions

def render_stats_section(class_data: Dict) -> Dict:
    """Render the stats section"""
    col1, col2 = st.columns(2)
    stats = {}
    
    with col1:
        st.write("**Base Stats**")
        stats.update({
            'base_hp': st.number_input("Base HP", value=class_data.get('base_hp', 0), key="base_hp"),
            'base_mp': st.number_input("Base MP", value=class_data.get('base_mp', 0), key="base_mp"),
            'base_physical_attack': st.number_input("Base Physical Attack", value=class_data.get('base_physical_attack', 0), key="base_pa"),
            'base_physical_defense': st.number_input("Base Physical Defense", value=class_data.get('base_physical_defense', 0), key="base_pd"),
            'base_agility': st.number_input("Base Agility", value=class_data.get('base_agility', 0), key="base_ag")
        })
    
    with col2:
        st.write("**Per Level Stats**")
        stats.update({
            'hp_per_level': st.number_input("HP per Level", value=class_data.get('hp_per_level', 0), key="hp_per_level"),
            'mp_per_level': st.number_input("MP per Level", value=class_data.get('mp_per_level', 0), key="mp_per_level"),
            'physical_attack_per_level': st.number_input("Physical Attack per Level", value=class_data.get('physical_attack_per_level', 0), key="pa_per_level"),
            'physical_defense_per_level': st.number_input("Physical Defense per Level", value=class_data.get('physical_defense_per_level', 0), key="pd_per_level"),
            'agility_per_level': st.number_input("Agility per Level", value=class_data.get('agility_per_level', 0), key="ag_per_level")
        })
    
    return stats