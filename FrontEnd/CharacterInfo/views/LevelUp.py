# ./FrontEnd/CharacterInfo/views/LevelUp.py

import streamlit as st
from typing import Tuple
from ..utils.database import (
    get_db_connection,
    get_available_classes_for_level_up
)

def level_up_class(character_id: int, class_id: int) -> Tuple[bool, str]:
    """Level up a character in a specific class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")

        # Check if character already has this class
        cursor.execute("""
            SELECT current_level 
            FROM character_class_progression
            WHERE character_id = ? AND class_id = ?
        """, (character_id, class_id))
        result = cursor.fetchone()

        if result:
            # Update existing progression
            cursor.execute("""
                UPDATE character_class_progression
                SET current_level = current_level + 1,
                    updated_at = DATETIME('now')
                WHERE character_id = ? AND class_id = ?
            """, (character_id, class_id))
        else:
            # Create new progression
            cursor.execute("""
                INSERT INTO character_class_progression (
                    character_id, class_id, current_level, current_experience
                ) VALUES (?, ?, 1, 0)
            """, (character_id, class_id))

        # Update character's total level
        cursor.execute("""
            UPDATE characters
            SET total_level = total_level + 1,
                updated_at = DATETIME('now')
            WHERE id = ?
        """, (character_id,))

        conn.commit()
        return True, "Level up successful!"
    except Exception as e:
        if conn:
            conn.rollback()
        return False, f"Error during level up: {str(e)}"
    finally:
        if conn:
            conn.close()

def render_level_up_interface(character_id: int):
    """Render the level up interface"""
    st.subheader("Level Up")
    
    # Load available classes fresh
    available_classes = get_available_classes_for_level_up(character_id)
    
    if not available_classes:
        st.warning("No classes available for level up!")
        return

    # Get unique categories and subcategories
    categories = sorted(set(c['category'] for c in available_classes if c['category']))
    class_types = sorted(set(c['type'] for c in available_classes if c['type']))
    
    # Filters with session state
    col1, col2, col3 = st.columns(3)
    
    def update_class_type():
        st.session_state.selected_class_type = st.session_state.class_type_filter
        
    def update_category():
        st.session_state.selected_category = st.session_state.category_filter
        
    def update_show_racial():
        st.session_state.show_racial = st.session_state.racial_filter
        
    def update_show_existing():
        st.session_state.show_existing = st.session_state.existing_filter
    
    with col1:
        type_options = ["All"] + class_types
        selected_type = st.selectbox(
            "Class Type",
            options=type_options,
            index=type_options.index(st.session_state.selected_class_type),
            key="class_type_filter",
            on_change=update_class_type
        )
    
    with col2:
        category_options = ["All"] + categories
        selected_category = st.selectbox(
            "Category",
            options=category_options,
            index=category_options.index(st.session_state.selected_category),
            key="category_filter",
            on_change=update_category
        )
    
    with col3:
        show_racial = st.checkbox(
            "Show Racial Classes",
            key="racial_filter",
            on_change=update_show_racial,
            value=st.session_state.show_racial
        )
        show_existing = st.checkbox(
            "Show Existing Classes",
            key="existing_filter",
            on_change=update_show_existing,
            value=st.session_state.show_existing
        )

    # Filter classes based on session state values
    filtered_classes = [
        c for c in available_classes
        if (st.session_state.selected_class_type == "All" or c['type'] == st.session_state.selected_class_type) and
           (st.session_state.selected_category == "All" or c['category'] == st.session_state.selected_category) and
           (st.session_state.show_racial or not c['is_racial']) and
           (st.session_state.show_existing or c['current_level'] == 0)
    ]

    # Display filtered classes
    if filtered_classes:
        # Create class selection radio buttons
        class_options = [
            f"{c['name']} (Level {c['current_level'] + 1})" 
            if c['current_level'] > 0 
            else f"{c['name']} (New Class)"
            for c in filtered_classes
        ]
        
        selected_index = st.radio(
            "Select Class to Level Up",
            range(len(class_options)),
            format_func=lambda x: class_options[x]
        )
        
        selected_class = filtered_classes[selected_index]
        
        # Show class details in a container
        with st.container():
            st.markdown("### Class Details")
            st.write(f"**Type:** {selected_class['type']}")
            st.write(f"**Category:** {selected_class['category'] or 'None'}")
            if selected_class['subcategory']:
                st.write(f"**Subcategory:** {selected_class['subcategory']}")
            st.write("**Description:**", selected_class['description'])
            st.write(f"**Current Level:** {selected_class['current_level']}")
            
            # Level up button
            if st.button("Confirm Level Up", use_container_width=True):
                success, message = level_up_class(character_id, selected_class['id'])
                if success:
                    st.success(message)
                    st.rerun()
                else:
                    st.error(message)
    else:
        st.warning("No classes match the selected filters.")

def render_level_up_tab():
    """Render the level up interface in its own tab"""
    st.subheader("Level Up Character")
    
    # Format display string for each character
    char_options = [
        f"{char[1]} {char[2]} (Level {char[3]}) - {char[4] or 'No Talent'} ({char[5]})" 
        for char in st.session_state.character_list
    ]
    
    selected_index = st.selectbox(
        "Select Character to Level Up",
        range(len(char_options)),
        format_func=lambda x: char_options[x]
    )
    
    if selected_index is not None:
        character_id = st.session_state.character_list[selected_index][0]
        render_level_up_interface(character_id)