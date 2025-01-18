# ./CharacterInfo.py

import streamlit as st
import sqlite3
from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple
from datetime import datetime

@dataclass
class Character:
    """Core character data"""
    id: int
    first_name: str
    middle_name: Optional[str]
    last_name: Optional[str]  # Made optional
    bio: Optional[str]
    total_level: int
    birth_place: Optional[str]
    age: Optional[int]
    karma: int
    talent: Optional[str]
    race_category: str  # Added race_category
    is_active: bool
    created_at: str
    updated_at: str

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def init_character_state():
    """Initialize character-related session state variables"""
    if 'current_character' not in st.session_state:
        st.session_state.current_character = None
    if 'character_list' not in st.session_state:
        st.session_state.character_list = []
        load_character_list()
    if 'show_new_race_form' not in st.session_state:
        st.session_state.show_new_race_form = False

def load_character_list():
    """Load list of available characters from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id, 
                c.first_name,
                COALESCE(c.last_name, ''),
                c.total_level,
                c.talent,
                c.race_category
            FROM characters c
            WHERE c.is_active = TRUE
            ORDER BY c.first_name, c.last_name
        """)
        st.session_state.character_list = cursor.fetchall()
    except Exception as e:
        st.error(f"Error loading character list: {str(e)}")
    finally:
        conn.close()

def load_character(character_id: int) -> Optional[Character]:
    """Load a character's basic information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                id,
                first_name,
                middle_name,
                last_name,
                bio,
                total_level,
                birth_place,
                age,
                karma,
                talent,
                race_category,
                is_active,
                created_at,
                updated_at
            FROM characters 
            WHERE id = ?
        """, (character_id,))
        result = cursor.fetchone()
        if result:
            # Convert tuple to Character object by unpacking
            return Character(*result)
        return None
    except Exception as e:
        st.error(f"Error loading character: {str(e)}")
        return None
    finally:
        conn.close()

def load_character_classes(character_id: int) -> List[Dict]:
    """Load character's class information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.is_racial,
                cp.current_level as level,
                cp.current_experience as exp
            FROM character_class_progression cp
            JOIN classes c ON cp.class_id = c.id
            WHERE cp.character_id = ?
            ORDER BY c.is_racial DESC, c.name
        """, (character_id,))
        columns = ['id', 'name', 'is_racial', 'level', 'exp']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        st.error(f"Error loading character classes: {str(e)}")
        return []
    finally:
        conn.close()  

def can_change_race_category(character_id: int) -> bool:
    """Check if character can change race category"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT COUNT(*) 
            FROM character_class_progression cp
            JOIN classes c ON cp.class_id = c.id
            WHERE cp.character_id = ? AND c.is_racial = TRUE
        """, (character_id,))
        count = cursor.fetchone()[0]
        return count == 0
    finally:
        conn.close()

def get_available_race_categories() -> List[str]:
    """Get list of available race categories"""
    return ["Humanoid", "Demi-Human", "Heteromorphic"]

def load_available_classes() -> List[tuple]:
    """Load list of available classes from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id, 
                c.name, 
                c.description, 
                c.class_type, 
                c.is_racial,
                c.category,
                c.subcategory
            FROM classes c
            ORDER BY c.is_racial DESC, c.class_type, c.category, c.name
        """)
        return cursor.fetchall()
    except Exception as e:
        st.error(f"Error loading available classes: {str(e)}")
        return []
    finally:
        conn.close()

def create_new_race(name: str, category: str, description: str) -> Tuple[bool, str]:
    """Create a new race class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            INSERT INTO classes (
                name, description, class_type, is_racial, 
                category, race_category
            ) VALUES (?, ?, 'Base', TRUE, ?, ?)
        """, (name, description, category, category))
        conn.commit()
        return True, "Race created successfully!"
    except Exception as e:
        return False, f"Error creating race: {str(e)}"
    finally:
        conn.close()

def get_available_classes_for_level_up(character_id: int) -> List[Dict]:
    """Get list of available classes for level up"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Get character's race category
        cursor.execute("""
            SELECT race_category, karma 
            FROM characters 
            WHERE id = ?
        """, (character_id,))
        race_category, karma = cursor.fetchone()

        # Get available classes based on prerequisites and karma
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.description,
                c.class_type,
                c.karma_requirement_min,
                c.karma_requirement_max
            FROM classes c
            WHERE 
                (c.is_racial = FALSE OR c.category = ?) AND
                ? BETWEEN c.karma_requirement_min AND c.karma_requirement_max AND
                NOT EXISTS (
                    SELECT 1 
                    FROM class_prerequisites p
                    WHERE p.class_id = c.id AND
                    NOT EXISTS (
                        SELECT 1 
                        FROM character_class_progression cp
                        WHERE cp.character_id = ? AND
                        cp.class_id = p.required_class_id AND
                        cp.current_level >= p.required_level
                    )
                )
            ORDER BY c.class_type, c.name
        """, (race_category, karma, character_id))

        columns = ['id', 'name', 'description', 'type', 'karma_min', 'karma_max']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

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
                SET current_level = current_level + 1
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
            SET total_level = total_level + 1
            WHERE id = ?
        """, (character_id,))

        cursor.execute("COMMIT")
        return True, "Level up successful!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error during level up: {str(e)}"
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

def render_level_up_interface(character_id: int):
    """Render the level up interface"""
    st.subheader("Level Up")
    
    available_classes = get_available_classes_for_level_up(character_id)
    
    if not available_classes:
        st.warning("No classes available for level up!")
        return

    # Group classes by type
    base_classes = [c for c in available_classes if c['type'] == 'Base']
    high_classes = [c for c in available_classes if c['type'] == 'High']
    rare_classes = [c for c in available_classes if c['type'] == 'Rare']

    col1, col2, col3 = st.columns(3)

    with col1:
        st.write("Base Classes")
        for c in base_classes:
            if st.button(f"Level {c['name']}", key=f"base_{c['id']}"):
                success, message = level_up_class(character_id, c['id'])
                if success:
                    st.success(message)
                    load_character_list()  # Refresh character list
                else:
                    st.error(message)

    with col2:
        st.write("High Classes")
        for c in high_classes:
            if st.button(f"Level {c['name']}", key=f"high_{c['id']}"):
                success, message = level_up_class(character_id, c['id'])
                if success:
                    st.success(message)
                    load_character_list()
                else:
                    st.error(message)

    with col3:
        st.write("Rare Classes")
        for c in rare_classes:
            if st.button(f"Level {c['name']}", key=f"rare_{c['id']}"):
                success, message = level_up_class(character_id, c['id'])
                if success:
                    st.success(message)
                    load_character_list()
                else:
                    st.error(message)

def render_character_view():
    """Display existing character information"""
    if st.session_state.character_list:
        # Format display string for each character
        char_options = [
            f"{char[1]} {char[2]} (Level {char[3]}) - {char[4] or 'No Talent'} ({char[5]})" 
            for char in st.session_state.character_list
        ]
        
        selected_index = st.selectbox(
            "Select Character",
            range(len(char_options)),
            format_func=lambda x: char_options[x]
        )
        
        if selected_index is not None:
            character_id = st.session_state.character_list[selected_index][0]
            character = load_character(character_id)
            
            if character:
                col1, col2 = st.columns(2)
                
                with col1:
                    st.subheader("Basic Information")
                    st.write(f"Name: {character.first_name} {character.middle_name or ''} {character.last_name or ''}")
                    st.write(f"Level: {character.total_level}")
                    st.write(f"Race Category: {character.race_category}")
                    
                    # Race category change
                    if can_change_race_category(character_id):
                        new_race_category = st.selectbox(
                            "Change Race Category",
                            get_available_race_categories(),
                            index=get_available_race_categories().index(character.race_category)
                        )
                        if new_race_category != character.race_category:
                            conn = get_db_connection()
                            cursor = conn.cursor()
                            try:
                                cursor.execute("""
                                    UPDATE characters 
                                    SET race_category = ? 
                                    WHERE id = ?
                                """, (new_race_category, character_id))
                                conn.commit()
                                st.success("Race category updated!")
                                load_character_list()  # Refresh character list
                            except Exception as e:
                                st.error(f"Error updating race category: {str(e)}")
                            finally:
                                conn.close()
                    
                    st.write(f"Talent: {character.talent}")
                    if character.birth_place:
                        st.write(f"Birth Place: {character.birth_place}")
                    if character.age:
                        st.write(f"Age: {character.age}")
                    if character.bio:
                        st.write("Biography:", character.bio)
                
                with col2:
                    st.subheader("Classes")
                    classes = load_character_classes(character_id)
                    
                    if classes:
                        # Display racial classes first
                        racial_classes = [c for c in classes if c['is_racial']]
                        if racial_classes:
                            st.write("Racial Classes:")
                            for c in racial_classes:
                                st.write(f"- {c['name']} [{c['level']}]")
                        
                        # Then display job classes
                        job_classes = [c for c in classes if not c['is_racial']]
                        if job_classes:
                            st.write("Job Classes:")
                            for c in job_classes:
                                st.write(f"- {c['name']} [{c['level']}]")
                    else:
                        st.write("No classes found")
                
                # Level Up section
                if st.button("Level Up"):
                    render_level_up_interface(character_id)
    else:
        st.info("No characters found. Create one in the 'Create Character' tab!")

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
                    race_category,  # Added race_category
                    selected_classes
                )
                if success:
                    st.success(message)
                    # Refresh character list
                    load_character_list()
                else:
                    st.error(message)

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

def render_character_tab():
    """Display character information and creation form"""
    st.header("Character Management")
    
    # Tabs for different character operations
    tab1, tab2 = st.tabs(["View Characters", "Create Character"])
    
    with tab1:
        if st.session_state.character_list:
            render_character_view()
        else:
            st.info("No characters found. Create one in the 'Create Character' tab!")
            
    with tab2:
        render_character_creation_form()