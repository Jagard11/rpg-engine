# ./FrontEnd/CharacterInfo/views/CharacterView.py

import streamlit as st
from typing import List, Tuple
from ..utils.database import (
    get_db_connection,
    load_character,
    load_character_classes,
    can_change_race_category
)

def get_available_race_categories() -> List[Tuple[int, str]]:
    """Get list of available race categories from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = TRUE 
            ORDER BY name
        """)
        return cursor.fetchall()
    finally:
        conn.close()

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
                    
                    # Get race category name
                    conn = get_db_connection()
                    cursor = conn.cursor()
                    try:
                        cursor.execute("""
                            SELECT name 
                            FROM class_categories 
                            WHERE id = ?
                        """, (character.race_category_id,))
                        race_category_name = cursor.fetchone()[0]
                        st.write(f"Race Category: {race_category_name}")
                    finally:
                        conn.close()
                    
                    # Race category change
                    if can_change_race_category(character_id):
                        race_categories = get_available_race_categories()
                        new_category_id = st.selectbox(
                            "Change Race Category",
                            options=[cat[0] for cat in race_categories],
                            format_func=lambda x: next(cat[1] for cat in race_categories if cat[0] == x),
                            index=next(i for i, cat in enumerate(race_categories) if cat[0] == character.race_category_id)
                        )
                        
                        if new_category_id != character.race_category_id:
                            conn = get_db_connection()
                            cursor = conn.cursor()
                            try:
                                cursor.execute("""
                                    UPDATE characters 
                                    SET race_category_id = ? 
                                    WHERE id = ?
                                """, (new_category_id, character_id))
                                conn.commit()
                                st.success("Race category updated!")
                                st.rerun()
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