# ./FrontEnd/CharacterInfo/views/JobClasses/forms.py

import streamlit as st
from typing import Dict, List
from .database import (
    get_class_types,
    get_class_categories,
    get_class_subcategories,
    get_all_classes
)

def render_basic_info(class_data: Dict) -> Dict:
    """Render and return basic class information form fields"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        name = st.text_input(
            "Name",
            value=class_data.get('name', '')
        )
    
    with col2:
        class_types = get_class_types()
        class_type = st.selectbox(
            "Type",
            options=[t["id"] for t in class_types],
            format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
            index=next(
                (i for i, t in enumerate(class_types) 
                 if t["id"] == class_data.get('class_type', 1)),
                0
            )
        )
    
    with col3:
        categories = get_class_categories()
        category = st.selectbox(
            "Category",
            options=[c["id"] for c in categories],
            format_func=lambda x: next(c["name"] for c in categories if c["id"] == x),
            index=next(
                (i for i, c in enumerate(categories) 
                 if c["id"] == class_data.get('category_id', 1)),
                0
            )
        )
    
    subcategories = get_class_subcategories()
    subcategory = st.selectbox(
        "Subcategory",
        options=[s["id"] for s in subcategories],
        format_func=lambda x: next(s["name"] for s in subcategories if s["id"] == x),
        index=next(
            (i for i, s in enumerate(subcategories) 
             if s["id"] == class_data.get('subcategory_id', 1)),
            0
        )
    )
    
    description = st.text_area(
        "Description",
        value=class_data.get('description', '')
    )
    
    return {
        'name': name,
        'class_type': class_type,
        'category_id': category,
        'subcategory_id': subcategory,
        'description': description
    }

def render_stats_tab(class_data: Dict) -> Dict:
    """Render and return class stats form fields"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.markdown("**Base Stats**")
        base_hp = st.number_input("Base HP", value=class_data.get('base_hp', 0))
        base_mp = st.number_input("Base MP", value=class_data.get('base_mp', 0))
        base_physical_attack = st.number_input("Base Physical Attack", value=class_data.get('base_physical_attack', 0))
        base_physical_defense = st.number_input("Base Physical Defense", value=class_data.get('base_physical_defense', 0))
        base_agility = st.number_input("Base Agility", value=class_data.get('base_agility', 0))
        base_magical_attack = st.number_input("Base Magical Attack", value=class_data.get('base_magical_attack', 0))
        base_magical_defense = st.number_input("Base Magical Defense", value=class_data.get('base_magical_defense', 0))
        base_resistance = st.number_input("Base Resistance", value=class_data.get('base_resistance', 0))
        base_special = st.number_input("Base Special", value=class_data.get('base_special', 0))

    with col2:
        st.markdown("**Per Level Stats (1/2)**")
        hp_per_level = st.number_input("HP per Level", value=class_data.get('hp_per_level', 0))
        mp_per_level = st.number_input("MP per Level", value=class_data.get('mp_per_level', 0))
        physical_attack_per_level = st.number_input("Physical Attack per Level", value=class_data.get('physical_attack_per_level', 0))
        physical_defense_per_level = st.number_input("Physical Defense per Level", value=class_data.get('physical_defense_per_level', 0))
        agility_per_level = st.number_input("Agility per Level", value=class_data.get('agility_per_level', 0))

    with col3:
        st.markdown("**Per Level Stats (2/2)**")
        magical_attack_per_level = st.number_input("Magical Attack per Level", value=class_data.get('magical_attack_per_level', 0))
        magical_defense_per_level = st.number_input("Magical Defense per Level", value=class_data.get('magical_defense_per_level', 0))
        resistance_per_level = st.number_input("Resistance per Level", value=class_data.get('resistance_per_level', 0))
        special_per_level = st.number_input("Special per Level", value=class_data.get('special_per_level', 0))
    
    return {
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

def render_prerequisite_tab():
    """Render prerequisite management interface"""
    st.subheader("Prerequisites")
    
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    
    # Add new group
    if st.form_submit_button("Add Prerequisite Group", use_container_width=True):
        st.session_state.prereq_groups.append([])
        st.rerun()
    
    # Render existing groups
    for group_idx, group in enumerate(st.session_state.prereq_groups):
        st.markdown(f"**Group {group_idx + 1}** (Meet any one)")
        
        # Add requirement to group
        col1, col2 = st.columns([3, 1])
        with col1:
            prereq_type = st.selectbox(
                "Type",
                options=["Class Level", "Category Total", "Subcategory Total", "Karma Range"],
                key=f"prereq_type_{group_idx}"
            )
        
        with col2:
            if st.form_submit_button(f"Add to Group {group_idx + 1}", use_container_width=True):
                st.session_state.prereq_groups[group_idx].append({
                    'type': prereq_type.lower().replace(' ', '_'),
                    'target_id': None,
                    'required_level': None,
                    'min_value': None,
                    'max_value': None
                })
                st.rerun()
        
        # Show requirements in group
        for req_idx, req in enumerate(group):
            cols = st.columns([3, 1])
            with cols[0]:
                if req['type'] == 'class_level':
                    classes = get_all_classes()
                    selected = st.selectbox(
                        "Class",
                        options=range(len(classes)),
                        format_func=lambda x: f"{classes[x]['name']} ({classes[x]['category']})",
                        key=f"class_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = classes[selected]['id']
                        req['required_level'] = st.number_input(
                            "Required Level",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"level_{group_idx}_{req_idx}"
                        )
                
                elif req['type'] in ['category_total', 'subcategory_total']:
                    items = get_class_categories() if req['type'] == 'category_total' else get_class_subcategories()
                    selected = st.selectbox(
                        f"{'Category' if req['type'] == 'category_total' else 'Subcategory'}",
                        options=range(len(items)),
                        format_func=lambda x: items[x]['name'],
                        key=f"cat_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = items[selected]['id']
                        req['required_level'] = st.number_input(
                            "Required Total Levels",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"total_{group_idx}_{req_idx}"
                        )
                
                elif req['type'] == 'karma_range':
                    req['min_value'] = st.number_input(
                        "Minimum Karma",
                        value=req.get('min_value', -1000),
                        key=f"karma_min_{group_idx}_{req_idx}"
                    )
                    req['max_value'] = st.number_input(
                        "Maximum Karma",
                        value=req.get('max_value', 1000),
                        key=f"karma_max_{group_idx}_{req_idx}"
                    )
            
            with cols[1]:
                if st.form_submit_button(f"Remove {req_idx + 1}", use_container_width=True):
                    st.session_state.prereq_groups[group_idx].pop(req_idx)
                    st.rerun()
                    
        if st.form_submit_button(f"Remove Group {group_idx + 1}", use_container_width=True):
            st.session_state.prereq_groups.pop(group_idx)
            st.rerun()
    
    return st.session_state.prereq_groups

def render_exclusion_tab():
    """Render exclusion management interface"""
    st.subheader("Exclusions")
    
    if 'exclusions' not in st.session_state:
        st.session_state.exclusions = []
    
    # Add new exclusion
    col1, col2 = st.columns([3, 1])
    with col1:
        excl_type = st.selectbox(
            "Type",
            options=["Class", "Category Total", "Subcategory Total", "Karma Range"]
        )
    
    with col2:
        if st.form_submit_button("Add Exclusion", use_container_width=True):
            st.session_state.exclusions.append({
                'type': excl_type.lower().replace(' ', '_'),
                'target_id': None,
                'min_value': None,
                'max_value': None
            })
            st.rerun()
    
    # Show existing exclusions
    for idx, excl in enumerate(st.session_state.exclusions):
        cols = st.columns([3, 1])
        with cols[0]:
            if excl['type'] == 'class':
                classes = get_all_classes()
                selected = st.selectbox(
                    "Excluded Class",
                    options=range(len(classes)),
                    format_func=lambda x: f"{classes[x]['name']} ({classes[x]['category']})",
                    key=f"excl_class_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = classes[selected]['id']
            
            elif excl['type'] in ['category_total', 'subcategory_total']:
                items = get_class_categories() if excl['type'] == 'category_total' else get_class_subcategories()
                selected = st.selectbox(
                    f"{'Category' if excl['type'] == 'category_total' else 'Subcategory'}",
                    options=range(len(items)),
                    format_func=lambda x: items[x]['name'],
                    key=f"excl_cat_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = items[selected]['id']
                    excl['min_value'] = st.number_input(
                        "Minimum Total Levels",
                        value=excl.get('min_value', 1),
                        key=f"excl_min_{idx}"
                    )
            
            elif excl['type'] == 'karma_range':
                excl['min_value'] = st.number_input(
                    "Min Karma",
                    value=excl.get('min_value', -1000),
                    key=f"excl_karma_min_{idx}"
                )
                excl['max_value'] = st.number_input(
                    "Max Karma",
                    value=excl.get('max_value', 1000),
                    key=f"excl_karma_max_{idx}"
                )
        
        with cols[1]:
            if st.form_submit_button(f"Remove Exclusion {idx + 1}", use_container_width=True):
                st.session_state.exclusions.pop(idx)
                st.rerun()
    
    return st.session_state.exclusions