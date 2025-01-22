# ./CharacterManagement/RaceEditor/interface.py

import streamlit as st
from typing import Optional
from .database import get_all_races, get_race_details, save_race, delete_race
from .forms import render_race_form

def render_race_editor():
    """Main interface for the race editor"""
    st.header("Race Editor")

    # Initialize session state
    if 'selected_race_id' not in st.session_state:
        st.session_state.selected_race_id = None
    if 'race_filter_category' not in st.session_state:
        st.session_state.race_filter_category = None
    if 'race_search_term' not in st.session_state:
        st.session_state.race_search_term = ""

    # Layout: Sidebar for race list, main area for editor
    col1, col2 = st.columns([1, 3])

    with col1:
        st.subheader("Race List")

        # Search and filter
        st.session_state.race_search_term = st.text_input(
            "Search Races",
            value=st.session_state.race_search_term
        )

        # Get and filter races
        all_races = get_all_races()
        filtered_races = [
            race for race in all_races
            if not st.session_state.race_search_term or 
            st.session_state.race_search_term.lower() in race['name'].lower()
        ]

        # Display race list
        st.write(f"Found {len(filtered_races)} races")
        
        for race in filtered_races:
            if st.button(
                f"{race['name']} ({race['category_name']})",
                key=f"race_{race['id']}",
                use_container_width=True,
                type="primary" if race['id'] == st.session_state.selected_race_id else "secondary"
            ):
                st.session_state.selected_race_id = race['id']
                st.rerun()

        # New race button
        if st.button("Create New Race", use_container_width=True):
            st.session_state.selected_race_id = None
            st.rerun()

    # Main editor area
    with col2:
        if st.session_state.selected_race_id is not None:
            race_data = get_race_details(st.session_state.selected_race_id)
            if not race_data:
                st.error("Selected race not found!")
                st.session_state.selected_race_id = None
                st.rerun()
        else:
            race_data = None

        # Show editor form
        form_data = render_race_form(race_data)
        
        # Handle form submission
        if form_data:
            success, message = save_race(form_data)
            if success:
                st.success(message)
                if not race_data:  # If this was a new race
                    st.session_state.selected_race_id = None
                st.rerun()
            else:
                st.error(message)

        # Delete button for existing races
        if st.session_state.selected_race_id:
            if st.button("Delete Race", type="secondary"):
                success, message = delete_race(st.session_state.selected_race_id)
                if success:
                    st.success(message)
                    st.session_state.selected_race_id = None
                    st.rerun()
                else:
                    st.error(message)