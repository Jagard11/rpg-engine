# ./CharacterManagement/CharacterEditor/forms/creation.py

import streamlit as st
from typing import Dict, Optional
from ..database import (
    get_available_race_categories,
    save_character
)

def render_character_creation_form(character_data: Optional[Dict] = None) -> None:
    """Render the character creation/editing form"""
    
    with st.form("character_form"):
        st.subheader("Basic Information")
        
        col1, col2, col3 = st.columns(3)
        with col1:
            first_name = st.text_input(
                "First Name",
                value=character_data.get('first_name', '') if character_data else ''
            )
        with col2:
            middle_name = st.text_input(
                "Middle Name",
                value=character_data.get('middle_name', '') if character_data else ''
            )
        with col3:
            last_name = st.text_input(
                "Last Name",
                value=character_data.get('last_name', '') if character_data else ''
            )
        
        # Race category
        race_categories = get_available_race_categories()
        
        # Find the index of the currently selected category
        selected_index = 0
        if character_data and character_data.get('race_category_id'):
            for i, cat in enumerate(race_categories):
                if cat['id'] == character_data['race_category_id']:
                    selected_index = i
                    break
        
        race_category_id = st.selectbox(
            "Race Category",
            options=[cat["id"] for cat in race_categories],
            format_func=lambda x: next(cat["name"] for cat in race_categories if cat["id"] == x),
            index=selected_index
        )
        
        # Optional fields
        col1, col2 = st.columns(2)
        with col1:
            birth_place = st.text_input(
                "Birth Place",
                value=character_data.get('birth_place', '') if character_data else ''
            )
            age = st.number_input(
                "Age",
                min_value=0,
                value=character_data.get('age', 0) if character_data else 0
            )
            
        with col2:
            talent = st.text_input(
                "Talent",
                value=character_data.get('talent', '') if character_data else ''
            )
        
        bio = st.text_area(
            "Biography",
            value=character_data.get('bio', '') if character_data else ''
        )
        
        # Submit button
        if st.form_submit_button("Save Character"):
            if not first_name:
                st.error("First name is required!")
                return
                
            data = {
                'id': character_data.get('id') if character_data else None,
                'first_name': first_name,
                'middle_name': middle_name if middle_name else None,
                'last_name': last_name if last_name else None,
                'race_category_id': race_category_id,
                'birth_place': birth_place if birth_place else None,
                'age': age if age > 0 else None,
                'talent': talent if talent else None,
                'bio': bio if bio else None
            }
            
            success, message = save_character(data)
            if success:
                st.success(message)
                st.rerun()
            else:
                st.error(message)