# ./CharacterManagement/RaceEditor/interface.py

import streamlit as st
from typing import Optional, Dict, List
from .database import get_all_races, get_race_details, save_race, delete_race
from .forms import render_race_form, get_class_types, get_race_categories, get_subcategories

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

        # Get filter options
        class_types = [{"id": None, "name": "All Types"}] + get_class_types()
        categories = [{"id": None, "name": "All Categories"}] + get_race_categories()
        subcategories = [{"id": None, "name": "All Subcategories"}] + get_subcategories()

        # Filter dropdowns
        selected_type = st.selectbox(
            "Class Type",
            options=[t["id"] for t in class_types],
            format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
            index=0
        )

        selected_category = st.selectbox(
            "Category",
            options=[c["id"] for c in categories],
            format_func=lambda x: next(c["name"] for c in categories if c["id"] == x),
            index=0
        )

        selected_subcategory = st.selectbox(
            "Subcategory",
            options=[s["id"] for s in subcategories],
            format_func=lambda x: next(s["name"] for s in subcategories if s["id"] == x),
            index=0
        )

        # Convert selected IDs to names for filtering
        selected_type_name = get_name_from_id(class_types, selected_type)
        selected_category_name = get_name_from_id(categories, selected_category)
        selected_subcategory_name = get_name_from_id(subcategories, selected_subcategory)

        # Get and filter races
        all_races = get_all_races()
        filtered_races = [
            race for race in all_races
            if (not st.session_state.race_search_term or 
                st.session_state.race_search_term.lower() in race['name'].lower()) and
            (selected_type_name in (None, "All Types") or race['type_name'] == selected_type_name) and
            (selected_category_name in (None, "All Categories") or race['category_name'] == selected_category_name) and
            (selected_subcategory_name in (None, "All Subcategories") or race['subcategory_name'] == selected_subcategory_name)
        ]

        # Display race list
        st.write(f"Found {len(filtered_races)} races")
        
        for race in filtered_races:
            col_race, col_delete = st.columns([5, 1])
            with col_race:
                if st.button(
                    f"{race['name']} ({race['category_name']})",
                    key=f"race_{race['id']}",
                    use_container_width=True,
                    type="primary" if race['id'] == st.session_state.selected_race_id else "secondary"
                ):
                    st.session_state.selected_race_id = race['id']
                    st.session_state.show_delete_confirm = False  # Reset delete confirmation
                    st.rerun()
            
            # Only show delete button for selected race
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

        # Copy button
        if st.session_state.selected_race_id:
            if st.button("Copy Race", type="secondary"):
                # Create a copy of race_data without the id
                copied_race = race_data.copy()
                copied_race.pop('id', None)
                copied_race['name'] = f"{copied_race['name']} (Copy)"
                
                # Save the copied race and get the new race details
                success, message = save_race(copied_race)
                if success:
                    # Find the newly created race by name
                    all_races = get_all_races()
                    new_race = next((race for race in all_races 
                                   if race['name'] == copied_race['name']), None)
                    if new_race:
                        st.session_state.selected_race_id = new_race['id']
                    st.success(message)
                    st.rerun()
                else:
                    st.error(message)