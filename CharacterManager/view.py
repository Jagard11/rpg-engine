# ./CharacterManagement/CharacterEditor/forms/view.py

import streamlit as st
from typing import Dict
from .database import get_character_classes

def render_character_view(character: Dict) -> None:
    """Render character details view"""
    st.subheader("Character Details")
    
    col1, col2 = st.columns(2)
    with col1:
        st.write(f"Name: {character.get('first_name', '')} {character.get('middle_name', '')} {character.get('last_name', '')}")
        st.write(f"Level: {character['total_level']}")
        st.write(f"Race Category: {character['race_category_name']}")
        if character.get('talent'):
            st.write(f"Talent: {character['talent']}")
        if character.get('birth_place'):
            st.write(f"Birth Place: {character['birth_place']}")
        if character.get('age'):
            st.write(f"Age: {character['age']}")
    
    with col2:
        classes = get_character_classes(character['id'])
        
        if classes:
            st.write("**Classes:**")
            
            # Display racial classes first
            racial_classes = [c for c in classes if c['is_racial']]
            if racial_classes:
                st.write("Racial Classes:")
                for c in racial_classes:
                    st.write(f"- {c['name']} (Level {c['level']})")
            
            # Then display job classes
            job_classes = [c for c in classes if not c['is_racial']]
            if job_classes:
                st.write("Job Classes:")
                for c in job_classes:
                    st.write(f"- {c['name']} (Level {c['level']})")
        else:
            st.write("No classes yet")
    
    if character.get('bio'):
        st.write("**Biography:**")
        st.write(character['bio'])