# ./FrontEnd/CharacterInfo/views/Shared/ClassEditor/forms.py

import streamlit as st
from typing import Dict, List, Optional, Tuple, Callable
from ...JobClasses.database import (
    get_class_types,
    get_class_categories,
    get_class_subcategories,
    get_all_classes
)

def render_basic_info(
    class_data: Dict,
    is_racial: bool = False,
    extra_fields: Optional[List[Tuple[str, Callable]]] = None
) -> Dict:
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
        categories = get_class_categories(is_racial)
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
    
    result = {
        'name': name,
        'class_type': class_type,
        'category_id': category,
        'subcategory_id': subcategory,
        'description': description,
        'is_racial': is_racial
    }
    
    # Add any extra fields
    if extra_fields:
        for field_name, render_func in extra_fields:
            result[field_name] = render_func()
            
    return result

def render_stats_section(class_data: Dict) -> Dict:
    """Render class stats section"""
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

def render_prerequisites_section(
    mode_prefix: str = "",
    include_racial: bool = False
) -> List[Dict]:
    """Render prerequisites interface fields without buttons
    
    Returns:
        List of prerequisite groups, each containing prerequisite dictionaries
    """
    st.subheader("Prerequisites")

    # Initialize session state if needed
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    
    # Create containers for each prerequisite group
    for group_idx, prereq_group in enumerate(st.session_state.prereq_groups):
        st.write(f"Group {group_idx + 1} (Any of these)")
        
        # Type selection for new requirements
        prereq_type_mapping = {
            "Class": "specific_class",
            "Category Total": "category_total",
            "Subcategory Total": "subcategory_total",
            "Karma": "karma"
        }
        
        st.selectbox(
            "Type",
            list(prereq_type_mapping.keys()),
            key=f"{mode_prefix}prereq_type_{group_idx}"
        )
        
        # Container for requirements in this group
        for req_idx, req in enumerate(prereq_group):
            st.markdown("---")
            
            if req['type'] == "specific_class":
                classes = get_all_classes(include_racial=include_racial)
                options = [f"{c['name']} ({c['category']})" for c in classes]
                selected = st.selectbox(
                    "Class",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"{mode_prefix}class_{group_idx}_{req_idx}"
                )
                if selected is not None:
                    req['target_id'] = classes[selected]['id']
                    req['required_level'] = st.number_input(
                        "Required Level",
                        min_value=1,
                        value=req.get('required_level', 1),
                        key=f"{mode_prefix}level_{group_idx}_{req_idx}"
                    )
            
            elif req['type'] in ["category_total", "subcategory_total"]:
                items = get_class_categories(is_racial=include_racial) if req['type'] == "category_total" else get_class_subcategories()
                options = [item['name'] for item in items]
                selected = st.selectbox(
                    f"{'Category' if req['type'] == 'category_total' else 'Subcategory'}",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"{mode_prefix}cat_{group_idx}_{req_idx}"
                )
                if selected is not None:
                    req['target_id'] = items[selected]['id']
                    req['required_level'] = st.number_input(
                        "Required Total Levels",
                        min_value=1,
                        value=req.get('required_level', 1),
                        key=f"{mode_prefix}total_{group_idx}_{req_idx}"
                    )
            
            elif req['type'] == "karma":
                req['min_value'] = st.number_input(
                    "Minimum Karma",
                    value=req.get('min_value', -1000),
                    key=f"{mode_prefix}karma_min_{group_idx}_{req_idx}"
                )
                req['max_value'] = st.number_input(
                    "Maximum Karma",
                    value=req.get('max_value', 1000),
                    key=f"{mode_prefix}karma_max_{group_idx}_{req_idx}"
                )
    
    return st.session_state.prereq_groups

def render_exclusions_section(
    mode_prefix: str = "",
    include_racial: bool = False
) -> List[Dict]:
    """Render exclusions interface fields without buttons
    
    Returns:
        List of exclusion dictionaries
    """
    st.subheader("Exclusions")
    
    # Initialize session state if needed
    if 'exclusions' not in st.session_state:
        st.session_state.exclusions = []
    
    # Type selection for new exclusions
    exclusion_type_mapping = {
        "Class": "specific_class",
        "Category Total": "category_total",
        "Subcategory Total": "subcategory_total",
        "Karma Range": "karma",
        "Racial Total": "racial_total"
    }
    
    st.selectbox(
        "Type",
        list(exclusion_type_mapping.keys()),
        key=f"{mode_prefix}excl_type"
    )
    
    # Container for existing exclusions
    for idx, excl in enumerate(st.session_state.exclusions):
        st.markdown("---")
        
        if excl['type'] == "specific_class":
            classes = get_all_classes(include_racial=include_racial)
            options = [f"{c['name']} ({c['category']})" for c in classes]
            selected = st.selectbox(
                "Excluded Class",
                range(len(options)),
                format_func=lambda x: options[x],
                key=f"{mode_prefix}excl_class_{idx}"
            )
            if selected is not None:
                excl['target_id'] = classes[selected]['id']
        
        elif excl['type'] in ["category_total", "subcategory_total"]:
            items = get_class_categories(is_racial=include_racial) if excl['type'] == "category_total" else get_class_subcategories()
            options = [item['name'] for item in items]
            selected = st.selectbox(
                f"{'Category' if excl['type'] == 'category_total' else 'Subcategory'}",
                range(len(options)),
                format_func=lambda x: options[x],
                key=f"{mode_prefix}excl_cat_{idx}"
            )
            if selected is not None:
                excl['target_id'] = items[selected]['id']
                excl['min_value'] = st.number_input(
                    "Minimum Total Levels",
                    value=excl.get('min_value', 1),
                    key=f"{mode_prefix}excl_min_{idx}"
                )
        
        elif excl['type'] == "karma":
            excl['min_value'] = st.number_input(
                "Min Karma",
                value=excl.get('min_value', -1000),
                key=f"{mode_prefix}excl_karma_min_{idx}"
            )
            excl['max_value'] = st.number_input(
                "Max Karma",
                value=excl.get('max_value', 1000),
                key=f"{mode_prefix}excl_karma_max_{idx}"
            )
        
        elif excl['type'] == "racial_total":
            excl['min_value'] = st.number_input(
                "Minimum Racial Levels",
                value=excl.get('min_value', 1),
                key=f"{mode_prefix}racial_min_{idx}"
            )
    
    return st.session_state.exclusions