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
    save_class,
    get_class_types,
    get_class_categories,
    get_class_subcategories,
    get_all_classes
)

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
            # Add prerequisite group button within the tab
            if st.form_submit_button(f"{mode_prefix}Add Prerequisite Group (OR)"):
                if 'prereq_groups' not in st.session_state:
                    st.session_state.prereq_groups = []
                st.session_state.prereq_groups.append([])
                st.rerun()

            # Prerequisite groups management
            for group_idx, prereq_group in enumerate(st.session_state.prereq_groups):
                st.markdown(f"#### Group {group_idx + 1} (Any of these)")

                # Type selection for new requirements
                prereq_type = st.selectbox(
                    "Prerequisite Type",
                    ["Class", "Category Total", "Subcategory Total", "Karma"],
                    key=f"{mode_prefix}prereq_type_{group_idx}"
                )

                # Add to group button
                if st.form_submit_button(f"{mode_prefix}Add to Group {group_idx + 1}"):
                    type_mapping = {
                        "Class": "specific_class",
                        "Category Total": "category_total",
                        "Subcategory Total": "subcategory_total",
                        "Karma": "karma"
                    }
                    st.session_state.prereq_groups[group_idx].append({
                        'type': type_mapping[prereq_type],
                        'target_id': None,
                        'required_level': None,
                        'min_value': None,
                        'max_value': None
                    })
                    st.rerun()

                # Display requirements in this group
                for req_idx, req in enumerate(prereq_group):
                    st.markdown("---")
                    col1, col2 = st.columns([3, 1])
                    
                    with col1:
                        if req['type'] == "specific_class":
                            classes = get_all_classes(include_racial=is_racial)
                            options = [c for c in classes]
                            selected_idx = st.selectbox(
                                "Class",
                                range(len(options)),
                                format_func=lambda x: f"{options[x]['name']} ({options[x]['category']})",
                                key=f"{mode_prefix}class_{group_idx}_{req_idx}"
                            )
                            if selected_idx is not None:
                                req['target_id'] = options[selected_idx]['id']
                                req['required_level'] = st.number_input(
                                    "Required Level",
                                    min_value=1,
                                    value=req.get('required_level', 1),
                                    key=f"{mode_prefix}level_{group_idx}_{req_idx}"
                                )
                        
                        elif req['type'] in ["category_total", "subcategory_total"]:
                            items = (get_class_categories(is_racial=is_racial) 
                                   if req['type'] == "category_total" 
                                   else get_class_subcategories())
                            selected_idx = st.selectbox(
                                "Category" if req['type'] == "category_total" else "Subcategory",
                                range(len(items)),
                                format_func=lambda x: items[x]['name'],
                                key=f"{mode_prefix}cat_{group_idx}_{req_idx}"
                            )
                            if selected_idx is not None:
                                req['target_id'] = items[selected_idx]['id']
                                req['required_level'] = st.number_input(
                                    "Required Total Levels",
                                    min_value=1,
                                    value=req.get('required_level', 1),
                                    key=f"{mode_prefix}total_{group_idx}_{req_idx}"
                                )
                        
                        elif req['type'] == "karma":
                            col1, col2 = st.columns(2)
                            with col1:
                                req['min_value'] = st.number_input(
                                    "Minimum Karma",
                                    value=req.get('min_value', -1000),
                                    key=f"{mode_prefix}karma_min_{group_idx}_{req_idx}"
                                )
                            with col2:
                                req['max_value'] = st.number_input(
                                    "Maximum Karma",
                                    value=req.get('max_value', 1000),
                                    key=f"{mode_prefix}karma_max_{group_idx}_{req_idx}"
                                )

                    with col2:
                        if st.form_submit_button(f"{mode_prefix}Remove Requirement {group_idx}-{req_idx}"):
                            st.session_state.prereq_groups[group_idx].pop(req_idx)
                            st.rerun()

                if st.form_submit_button(f"{mode_prefix}Remove Group {group_idx + 1}"):
                    st.session_state.prereq_groups.pop(group_idx)
                    st.rerun()
            
            # Store prerequisites data
            prereqs = st.session_state.prereq_groups
        
        with excl_tab:
            # Render exclusions section
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
                    st.rerun()
                else:
                    st.error(message)
            
            except Exception as e:
                st.error(f"Error saving changes: {str(e)}")