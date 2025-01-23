# ./CharacterManagement/RaceEditor/interface.py

import streamlit as st
from typing import Optional, Dict, List
from .database import get_all_races, get_race_details, save_race, delete_race
from .forms import (
    render_race_form,
    get_class_types,
    get_race_categories,
    get_subcategories
)

def get_name_from_id(items: List[Dict], target_id: Optional[int]) -> Optional[str]:
    """Helper function to get name from id in a list of dicts"""
    if target_id is None:
        return None
    return next((item["name"] for item in items if item["id"] == target_id), None)

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
    if 'show_delete_confirm' not in st.session_state:
        st.session_state.show_delete_confirm = False

    # Layout: Sidebar for race list, main area for editor
    col1, col2 = st.columns([1, 3])

    with col1:
        st.subheader("Race List")
        
        # Search and filter section
        st.session_state.race_search_term = st.text_input(
            "Search Races",
            value=st.session_state.race_search_term
        )

        # Display race list
        all_races = get_all_races()
        for race in all_races:
            col_race, col_delete = st.columns([5, 1])
            with col_race:
                if st.button(
                    f"{race['name']} ({race['category_name']})",
                    key=f"race_{race['id']}",
                    use_container_width=True,
                    type="primary" if race['id'] == st.session_state.selected_race_id else "secondary"
                ):
                    st.session_state.selected_race_id = race['id']
                    st.session_state.show_delete_confirm = False
                    st.rerun()

            with col_delete:
                if race['id'] == st.session_state.selected_race_id:
                    if not st.session_state.show_delete_confirm:
                        if st.button("🗑️", key=f"delete_{race['id']}", type="secondary"):
                            st.session_state.show_delete_confirm = True
                            st.rerun()
                    else:
                        if st.button("❌", key=f"cancel_delete_{race['id']}", type="secondary"):
                            st.session_state.show_delete_confirm = False
                            st.rerun()
                        if st.button("✅", key=f"confirm_delete_{race['id']}", type="secondary"):
                            success, message = delete_race(race['id'])
                            if success:
                                st.success(message)
                                st.session_state.selected_race_id = None
                                st.session_state.show_delete_confirm = False
                                st.rerun()
                            else:
                                st.error(message)
                                st.session_state.show_delete_confirm = False

        # New race button
        if st.button("Create New Race", use_container_width=True):
            st.session_state.selected_race_id = None
            st.session_state.show_delete_confirm = False
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
        
        if form_data:
            success, message = save_race(form_data)
            if success:
                st.success(message)
                if not race_data:  # If this was a new race
                    st.session_state.selected_race_id = None
                st.rerun()
            else:
                st.error(message)