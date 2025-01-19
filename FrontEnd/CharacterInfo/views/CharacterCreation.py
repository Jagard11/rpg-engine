# ./FrontEnd/CharacterInfo/views/CharacterCreation.py

import streamlit as st
from typing import List, Tuple
from ..utils.database import get_db_connection, load_available_classes

def get_available_race_categories() -> List[str]:
    """Get list of available race categories"""
    return ["Humanoid", "Demi-Human", "Heteromorphic"]

def create_new_race(name: str, category: str, description: str) -> Tuple[bool, str]:
    """Create a new race class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # First get the category_id from class_categories
        cursor.execute("""
            SELECT id FROM class_categories 
            WHERE name = ? AND is_racial = TRUE
        """, (category,))
        category_result = cursor.fetchone()
        if not category_result:
            return False, f"Race category {category} not found"
        
        category_id = category_result[0]
        
        # Get a default subcategory for this category
        cursor.execute("""
            SELECT id FROM class_subcategories 
            WHERE name = 'Magekin'
        """)
        subcategory_result = cursor.fetchone()
        if not subcategory_result:
            return False, "Default subcategory not found"
            
        subcategory_id = subcategory_result[0]
        
        # Insert the new race class
        cursor.execute("""
            INSERT INTO classes (
                name, description, class_type, is_racial, 
                category_id, subcategory_id,
                base_hp, base_mp, base_physical_attack, base_physical_defense,
                base_agility, base_magical_attack, base_magical_defense,
                base_resistance, base_special
            ) VALUES (
                ?, ?, 1, TRUE, 
                ?, ?,
                10, 10, 10, 10,
                10, 10, 10,
                10, 10
            )
        """, (name, description, category_id, subcategory_id))
        
        conn.commit()
        return True, "Race created successfully!"
    except Exception as e:
        return False, f"Error creating race: {str(e)}"
    finally:
        conn.close()
        
def create_character(first_name, middle_name, last_name, bio, birth_place, age, talent, race_category, selected_classes):
    """Create a new character with selected classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Start transaction
        cursor.execute("BEGIN TRANSACTION")
        
        # Insert character
        cursor.execute("""
            INSERT INTO characters (
                first_name, middle_name, last_name, bio, birth_place, 
                age, talent, race_category, total_level, karma, is_active
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, 0, TRUE)
        """, (first_name, middle_name, last_name, bio, birth_place, age, talent, race_category))
        
        character_id = cursor.lastrowid
        
        # Insert class progressions
        for class_id, level in selected_classes:
            cursor.execute("""
                INSERT INTO character_class_progression (
                    character_id, class_id, current_level, current_experience
                ) VALUES (?, ?, ?, 0)
            """, (character_id, class_id, level))
            
        # Update total level
        total_level = sum(level for _, level in selected_classes)
        cursor.execute("""
            UPDATE characters 
            SET total_level = ? 
            WHERE id = ?
        """, (total_level, character_id))
        
        # Create initial stats
        cursor.execute("""
            INSERT INTO character_stats (
                character_id,
                base_hp, current_hp,
                base_mp, current_mp,
                base_physical_attack, current_physical_attack,
                base_physical_defense, current_physical_defense,
                base_agility, current_agility,
                base_magical_attack, current_magical_attack,
                base_magical_defense, current_magical_defense,
                base_resistance, current_resistance,
                base_special, current_special
            ) VALUES (?, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        """, (character_id,))
        
        # Commit transaction
        cursor.execute("COMMIT")
        return True, "Character created successfully!"
        
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error creating character: {str(e)}"
    finally:
        conn.close()

def render_new_race_form():
    """Render form for creating a new race"""
    st.subheader("Create New Race")
    
    with st.form("new_race_form"):
        name = st.text_input("Race Name")
        category = st.selectbox("Race Category", get_available_race_categories())
        description = st.text_area("Description")
        
        if st.form_submit_button("Create Race"):
            if name and category:
                success, message = create_new_race(name, category, description)
                if success:
                    st.success(message)
                    st.session_state.show_new_race_form = False
                else:
                    st.error(message)
            else:
                st.error("Name and category are required!")

def render_character_creation_form():
    """Render the character creation form"""
    st.subheader("Create New Character")
    
    # Add button to show/hide new race form
    if st.button("Add New Race"):
        st.session_state.show_new_race_form = not st.session_state.show_new_race_form

    if st.session_state.show_new_race_form:
        render_new_race_form()
        st.divider()
    
    with st.form("character_creation"):
        col1, col2, col3 = st.columns(3)
        
        with col1:
            first_name = st.text_input("First Name", key="first_name")
        with col2:
            middle_name = st.text_input("Middle Name", key="middle_name")
        with col3:
            last_name = st.text_input("Last Name (Optional)", key="last_name")
            
        bio = st.text_area("Biography", key="bio")
        
        col1, col2, col3 = st.columns(3)
        with col1:
            birth_place = st.text_input("Place of Birth", key="birth_place")
        with col2:
            age = st.number_input("Age", min_value=0, key="age")
        with col3:
            race_category = st.selectbox("Race Category", get_available_race_categories(), key="race_category")
            
        talent = st.text_input("Talent", key="talent")
        
        # Class selection
        st.subheader("Select Classes")
        available_classes = load_available_classes()
        selected_classes = []
        
        # Group classes by type
        racial_classes = [c for c in available_classes if c[4]]  # is_racial
        job_classes = [c for c in available_classes if not c[4]]  # not is_racial
        
        # Racial class selection
        st.write("Racial Classes")
        for class_info in racial_classes:
            col1, col2 = st.columns([3, 1])
            with col1:
                if st.checkbox(f"{class_info[1]} - {class_info[2]}", key=f"class_{class_info[0]}"):
                    with col2:
                        level = st.number_input(
                            "Level",
                            min_value=1,
                            max_value=100,
                            value=1,
                            key=f"level_{class_info[0]}"
                        )
                        selected_classes.append((class_info[0], level))
        
        # Job class selection
        st.write("Job Classes")
        for class_info in job_classes:
            col1, col2 = st.columns([3, 1])
            with col1:
                if st.checkbox(f"{class_info[1]} - {class_info[2]}", key=f"class_{class_info[0]}"):
                    with col2:
                        level = st.number_input(
                            "Level",
                            min_value=1,
                            max_value=100,
                            value=1,
                            key=f"level_{class_info[0]}"
                        )
                        selected_classes.append((class_info[0], level))
        
        # Submit button
        if st.form_submit_button("Create Character"):
            if not first_name:
                st.error("First name is required!")
            elif not selected_classes:
                st.error("Please select at least one class!")
            else:
                success, message = create_character(
                    first_name, middle_name, last_name,
                    bio, birth_place, age, talent,
                    race_category,
                    selected_classes
                )
                if success:
                    st.success(message)
                    st.rerun()  # Refresh the page
                else:
                    st.error(message)