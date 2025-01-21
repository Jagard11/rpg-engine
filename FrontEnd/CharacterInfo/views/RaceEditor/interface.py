# ./FrontEnd/CharacterInfo/views/RaceEditor/interface.py

import streamlit as st
from typing import List, Tuple, Dict, Optional
from ..Shared.ClassEditor import render_class_editor
from ...utils.database import get_db_connection

def get_filtered_races(
    search: str = "",
    category_id: Optional[int] = None
) -> List[Dict]:
    """Get filtered list of racial classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Build query conditions
        conditions = ["c.is_racial = TRUE"]
        params = []

        if search:
            conditions.append("(c.name LIKE ? OR c.description LIKE ?)")
            search_term = f"%{search}%"
            params.extend([search_term, search_term])

        if category_id:
            conditions.append("c.category_id = ?")
            params.append(category_id)

        # Construct and execute query
        query = f"""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE {" AND ".join(conditions)}
            ORDER BY cc.name, c.name
        """
        
        cursor.execute(query, params)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def render_race_editor():
    """Render the race editor interface"""
    st.header("Race Editor")

    # Initialize session state
    if 'selected_race_id' not in st.session_state:
        st.session_state.selected_race_id = None

    # Load available races
    col1, col2 = st.columns([1, 3])
    
    with col1:
        st.subheader("Available Races")
        
        # Search box
        search = st.text_input("Search Races", key="race_search")
        
        # Category filter
        categories = [(None, "All Categories")]
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = TRUE 
            ORDER BY name
        """)
        categories.extend(cursor.fetchall())
        conn.close()
        
        category_id = st.selectbox(
            "Category",
            options=[c[0] for c in categories],
            format_func=lambda x: next(c[1] for c in categories if c[0] == x),
            key="race_category_filter"
        )

        # Get filtered races
        races = get_filtered_races(search=search, category_id=category_id)
        
        # Display races list
        if races:
            for race in races:
                if st.button(
                    f"{race['name']} ({race['category_name']})",
                    key=f"race_{race['id']}",
                    use_container_width=True,
                    type="secondary" if race['id'] != st.session_state.selected_race_id else "primary"
                ):
                    st.session_state.selected_race_id = race['id']
                    # Clear editor state
                    if 'prereq_groups' in st.session_state:
                        del st.session_state.prereq_groups
                    if 'exclusions' in st.session_state:
                        del st.session_state.exclusions
                    st.rerun()
        else:
            st.write("No races found matching filters")

        # New race button
        if st.button("Create New Race", use_container_width=True):
            st.session_state.selected_race_id = None
            # Clear editor state
            if 'prereq_groups' in st.session_state:
                del st.session_state.prereq_groups
            if 'exclusions' in st.session_state:
                del st.session_state.exclusions
            st.rerun()

    # Editor section
    with col2:
        # Extra fields specific to races
        extra_fields = []
        
        # Render shared class editor
        render_class_editor(
            class_id=st.session_state.selected_race_id,
            is_racial=True,
            extra_fields=extra_fields,
            mode_prefix="race_"
        )