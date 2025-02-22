# ./ClassManager/JobClassEditor/basic_info_tab.py

import streamlit as st
from typing import Dict, Any
from .utils import get_class_spell_schools  # Import from utils

def render_basic_info_tab(current_record: Dict[str, Any], class_types: Dict[int, str], 
                         categories: Dict[int, str], subcategories: Dict[int, str]) -> Dict[str, Any]:
    """Render the Basic Information tab and return its data"""
    st.subheader("Basic Information")
    st.number_input("ID", value=st.session_state.current_class_id, disabled=True, key="class_id_input")
    name = st.text_input("Name", value=current_record.get('name', ''), key="class_name_input")
    description = st.text_area("Description", value=current_record.get('description', ''), key="class_description_input")

    current_class_type = current_record.get('class_type', list(class_types.keys())[0] if class_types else None)
    class_type = st.selectbox(
        "Class Type",
        options=list(class_types.keys()),
        format_func=lambda x: class_types.get(x, ''),
        key="class_type_input",
        index=list(class_types.keys()).index(current_class_type) if current_class_type in class_types else 0,
        help="Suggested: Base (15 levels), High (10 levels), Rare (5 levels)"
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
        index=list(subcategories.keys()).index(current_subcategory) if current_subcategory in subcategories else 0,
        help="For Race Classes, may represent creature type (e.g., Humanoid, Undead)"
    )
    is_racial = st.checkbox("Is Racial Class", value=current_record.get('is_racial', False), key="is_racial_input")

    if st.session_state.current_class_id != 0:
        spell_schools = get_class_spell_schools(st.session_state.current_class_id)
        if spell_schools:
            st.write(f"Spell Schools (from assigned spells): {', '.join(spell_schools)}")
        else:
            st.write("No spells assigned yet. Add via Spell List Editor in the Spell List tab.")

    return {
        'name': name,
        'description': description,
        'class_type': class_type,
        'category_id': category,
        'subcategory_id': subcategory,
        'is_racial': is_racial
    }