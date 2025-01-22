# ./CharacterManagement/CharacterEditor/interface.py

import streamlit as st
from .database import get_characters, get_character_details
from .forms import render_character_creation_form, render_character_view

def render_character_editor():
    """Main interface for the character editor"""
    st.header("Character Editor")

    # Initialize session state
    if 'selected_character_id' not in st.session_state:
        st.session_state.selected_character_id = None
    if 'character_search_term' not in st.session_state:
        st.session_state.character_search_term = ""

    # Layout: Sidebar for character list, main area for editor
    col1, col2 = st.columns([1, 3])

    with col1:
        st.subheader("Characters")

        # Search
        st.session_state.character_search_term = st.text_input(
            "Search Characters",
            value=st.session_state.character_search_term
        )

        # Get and filter characters
        characters = get_characters()
        filtered_characters = [
            char for char in characters
            if not st.session_state.character_search_term or 
            st.session_state.character_search_term.lower() in char['name'].lower()
        ]

        # Display character list
        st.write(f"Found {len(filtered_characters)} characters")
        
        for char in filtered_characters:
            if st.button(
                f"{char['name']} (Level {char['total_level']})",
                key=f"char_{char['id']}",
                use_container_width=True,
                type="primary" if char['id'] == st.session_state.selected_character_id else "secondary"
            ):
                st.session_state.selected_character_id = char['id']
                st.rerun()

        # New character button
        if st.button("Create New Character", use_container_width=True):
            st.session_state.selected_character_id = None
            st.rerun()

    # Main editor area
    with col2:
        if st.session_state.selected_character_id is not None:
            character = get_character_details(st.session_state.selected_character_id)
            if not character:
                st.error("Selected character not found!")
                st.session_state.selected_character_id = None
                st.rerun()
                
            # Show character details
            render_character_view(character)
        else:
            # Show character creation form
            render_character_creation_form()