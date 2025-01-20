# ./FrontEnd/CharacterInfo/views/JobClassBrowser.py

import streamlit as st
from typing import Dict, List, Optional, Tuple
from ..utils.database import get_db_connection

def get_filtered_classes(
    search: str = "",
    category_id: Optional[int] = None,
    class_type: Optional[str] = None,
    show_racial: bool = True,
    show_prerequisites: bool = False
) -> List[Dict]:
    """Get filtered list of classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Build query conditions
        conditions = ["1=1"]  # Always true condition to start
        params = []

        if search:
            conditions.append("(c.name LIKE ? OR c.description LIKE ?)")
            search_term = f"%{search}%"
            params.extend([search_term, search_term])

        if category_id:
            conditions.append("c.category_id = ?")
            params.append(category_id)

        if class_type:
            conditions.append("ct.name = ?")
            params.append(class_type)

        if not show_racial:
            conditions.append("c.is_racial = 0")

        if show_prerequisites:
            conditions.append("""
                EXISTS (
                    SELECT 1 FROM class_prerequisites cp 
                    WHERE cp.class_id = c.id
                )
            """)

        # Construct and execute query
        query = f"""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name,
                c.is_racial,
                c.base_hp,
                c.base_mp,
                c.base_physical_attack,
                c.base_physical_defense,
                c.base_agility,
                c.base_magical_attack,
                c.base_magical_defense,
                c.base_resistance,
                c.base_special,
                c.hp_per_level,
                c.mp_per_level,
                c.physical_attack_per_level,
                c.physical_defense_per_level,
                c.agility_per_level,
                c.magical_attack_per_level,
                c.magical_defense_per_level,
                c.resistance_per_level,
                c.special_per_level
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE {" AND ".join(conditions)}
            ORDER BY c.is_racial DESC, cc.name, c.name
        """
        
        cursor.execute(query, params)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_prerequisites(class_id: int) -> List[Dict]:
    """Get prerequisites for a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        query = """
            SELECT 
                cp.prerequisite_type,
                cp.prerequisite_group,
                cp.target_id,
                cp.required_level,
                cp.min_value,
                cp.max_value,
                c2.name as target_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM class_prerequisites cp
            LEFT JOIN classes c2 ON cp.target_id = c2.id
            LEFT JOIN class_categories cc ON cp.target_id = cc.id
            LEFT JOIN class_subcategories cs ON cp.target_id = cs.id
            WHERE cp.class_id = ?
            ORDER BY cp.prerequisite_group, cp.id
        """
        cursor.execute(query, (class_id,))
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_exclusions(class_id: int) -> List[Dict]:
    """Get exclusions for a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        query = """
            SELECT 
                ce.exclusion_type,
                ce.target_id,
                ce.min_value,
                ce.max_value,
                c2.name as target_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM class_exclusions ce
            LEFT JOIN classes c2 ON ce.target_id = c2.id
            LEFT JOIN class_categories cc ON ce.target_id = cc.id
            LEFT JOIN class_subcategories cs ON ce.target_id = cs.id
            WHERE ce.class_id = ?
        """
        cursor.execute(query, (class_id,))
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def render_class_card(class_data: Dict):
    """Render a class card with collapsible details"""
    with st.expander(f"{class_data['name']} ({class_data['type_name']})"):
        # Basic info
        st.markdown(f"**Category:** {class_data['category_name']}")
        if class_data['subcategory_name']:
            st.markdown(f"**Subcategory:** {class_data['subcategory_name']}")
        
        # Description
        if class_data['description']:
            st.markdown(f"**Description:** {class_data['description']}")
        
        # Stats in columns
        col1, col2, col3 = st.columns(3)
        
        with col1:
            st.markdown("**Base Stats:**")
            st.write(f"HP: {class_data['base_hp']}")
            st.write(f"MP: {class_data['base_mp']}")
            st.write(f"Physical Attack: {class_data['base_physical_attack']}")
            st.write(f"Physical Defense: {class_data['base_physical_defense']}")
            st.write(f"Agility: {class_data['base_agility']}")

        with col2:
            st.markdown("&nbsp;")  # Spacing for alignment
            st.write(f"Magical Attack: {class_data['base_magical_attack']}")
            st.write(f"Magical Defense: {class_data['base_magical_defense']}")
            st.write(f"Resistance: {class_data['base_resistance']}")
            st.write(f"Special: {class_data['base_special']}")

        with col3:
            st.markdown("**Per Level Gains:**")
            st.write(f"HP: +{class_data['hp_per_level']}")
            st.write(f"MP: +{class_data['mp_per_level']}")
            st.write(f"Physical Attack: +{class_data['physical_attack_per_level']}")
            st.write(f"Physical Defense: +{class_data['physical_defense_per_level']}")
            st.write(f"Agility: +{class_data['agility_per_level']}")

        # Prerequisites and Exclusions
        st.markdown("---")
        col1, col2 = st.columns(2)
        
        with col1:
            prerequisites = get_class_prerequisites(class_data['id'])
            if prerequisites:
                st.markdown("**Prerequisites:**")
                # Group prerequisites by prerequisite_group
                prereq_groups = {}
                for prereq in prerequisites:
                    group = prereq['prerequisite_group']
                    if group not in prereq_groups:
                        prereq_groups[group] = []
                    prereq_groups[group].append(prereq)
                
                # Display each group
                for group_prereqs in prereq_groups.values():
                    prereq_texts = []
                    for prereq in group_prereqs:
                        if prereq['prerequisite_type'] == 'specific_class':
                            text = f"{prereq['target_name']} (Level {prereq['required_level']})"
                        elif prereq['prerequisite_type'] == 'karma':
                            text = f"Karma between {prereq['min_value']} and {prereq['max_value']}"
                        else:
                            text = f"{prereq['category_name'] or prereq['subcategory_name']} Total Level {prereq['required_level']}"
                        prereq_texts.append(text)
                    st.write(" OR ".join(prereq_texts))
        
        with col2:
            exclusions = get_class_exclusions(class_data['id'])
            if exclusions:
                st.markdown("**Exclusions:**")
                for excl in exclusions:
                    if excl['exclusion_type'] == 'specific_class':
                        st.write(f"• Cannot have: {excl['target_name']}")
                    elif excl['exclusion_type'] == 'karma':
                        st.write(f"• Cannot have karma between {excl['min_value']} and {excl['max_value']}")
                    elif excl['exclusion_type'] == 'racial_total':
                        st.write(f"• Cannot have {excl['min_value']}+ racial levels")
                    else:
                        st.write(f"• Cannot have {excl['min_value']}+ levels in {excl['category_name'] or excl['subcategory_name']}")

def render_job_classes_browser():
    """Render the job classes browser interface"""
    st.header("Class Browser")
    
    # Filters in the sidebar
    st.sidebar.header("Filters")
    
    # Search
    search = st.sidebar.text_input("Search Classes", key="class_search")
    
    # Category filter
    categories = [(None, "All Categories")]  # Start with "All" option
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT id, name FROM class_categories ORDER BY name")
    categories.extend(cursor.fetchall())
    conn.close()
    
    category_id = st.sidebar.selectbox(
        "Category",
        options=[c[0] for c in categories],
        format_func=lambda x: next(c[1] for c in categories if c[0] == x),
        key="category_filter"
    )
    
    # Class type filter
    class_types = [(None, "All Types")]  # Start with "All" option
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM class_types ORDER BY name")
    class_types.extend((name[0], name[0]) for name in cursor.fetchall())
    conn.close()
    
    class_type = st.sidebar.selectbox(
        "Class Type",
        options=[t[0] for t in class_types],
        format_func=lambda x: next(t[1] for t in class_types if t[0] == x),
        key="type_filter"
    )
    
    # Additional filters
    show_racial = st.sidebar.checkbox("Show Racial Classes", value=True, key="show_racial")
    show_prerequisites = st.sidebar.checkbox("Has Prerequisites", key="show_prerequisites")
    
    # Get and display filtered classes
    classes = get_filtered_classes(
        search=search,
        category_id=category_id,
        class_type=class_type,
        show_racial=show_racial,
        show_prerequisites=show_prerequisites
    )
    
    # Display results count
    st.write(f"Found {len(classes)} classes")
    
    # Display classes
    for class_data in classes:
        render_class_card(class_data)