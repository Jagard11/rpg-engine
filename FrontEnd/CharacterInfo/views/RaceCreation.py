# ./FrontEnd/CharacterInfo/views/RaceCreation.py

import streamlit as st
from typing import List, Tuple
from ..utils.database import get_db_connection

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

def create_new_race(name: str, category_id: int, description: str) -> Tuple[bool, str]:
    """Create a new race class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Get subcategory for the race
        cursor.execute("""
            SELECT id FROM class_subcategories 
            WHERE name = 'Magekin' 
            LIMIT 1
        """)
        subcategory_id = cursor.fetchone()[0]

        # Get class type for the race (base)
        cursor.execute("SELECT id FROM class_types WHERE name = 'base' LIMIT 1")
        class_type_id = cursor.fetchone()[0]

        cursor.execute("""
            INSERT INTO classes (
                name, description, class_type, is_racial, 
                category_id, subcategory_id
            ) VALUES (?, ?, ?, TRUE, ?, ?)
        """, (name, description, class_type_id, category_id, subcategory_id))
        conn.commit()
        return True, "Race created successfully!"
    except Exception as e:
        return False, f"Error creating race: {str(e)}"
    finally:
        conn.close()

def verify_race_exists(name: str) -> bool:
    """Verify that a race exists in the database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT COUNT(*) FROM classes 
            WHERE name = ? AND is_racial = TRUE
        """, (name,))
        count = cursor.fetchone()[0]
        return count > 0
    finally:
        conn.close()

def clear_success_banner():
    """Clear the success banner after timeout"""
    st.session_state.show_success_banner = False

def render_race_creation_form():
    """Render form for creating a new race"""
    st.subheader("Create New Race")
    
    # Initialize session state for banner visibility if not exists
    if 'show_success_banner' not in st.session_state:
        st.session_state.show_success_banner = False
    if 'success_race_name' not in st.session_state:
        st.session_state.success_race_name = ""
    
    # Get race categories
    race_categories = get_available_race_categories()
    
    # Create two columns - one for form, one for success banner
    form_col, banner_col = st.columns([2, 1])
    
    with form_col:
        # Form inputs
        name = st.text_input("Race Name")
        category_id = st.selectbox(
            "Race Category",
            options=[cat[0] for cat in race_categories],
            format_func=lambda x: next(cat[1] for cat in race_categories if cat[0] == x)
        )
        description = st.text_area("Description")
        
        if st.button("Create Race"):
            if name and category_id:
                success, message = create_new_race(name, category_id, description)
                if success:
                    # Verify the race was actually added
                    if verify_race_exists(name):
                        st.session_state.show_success_banner = True
                        st.session_state.success_race_name = name
                        # Schedule banner removal after 10 seconds
                        st.rerun()
                    else:
                        st.error("Race creation appeared successful but verification failed. Please try again.")
                else:
                    st.error(message)
            else:
                st.error("Name and category are required!")
    
    # Show success banner in the right column if active
    with banner_col:
        if st.session_state.show_success_banner:
            st.markdown("""
            <div style="padding: 1rem; background-color: #28a745; border-radius: 0.5rem; margin: 1rem 0;">
                <p style="color: white; font-size: 1.1rem; margin: 0;">
                    ✅ Race Successfully Added!
                </p>
            </div>
            """, unsafe_allow_html=True)
            
            # Add race details under the banner
            st.markdown(f"**{st.session_state.success_race_name}** has been added to the database.")
            
            # Schedule the banner to disappear
            import time
            time.sleep(0.1)  # Small delay to ensure banner shows
            st.session_state.show_success_banner = False