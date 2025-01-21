# ./FrontEnd/CharacterInfo/views/JobClassEditor/interface.py

import streamlit as st
from typing import Dict, List, Optional
from ..Shared.ClassEditor import render_class_editor
from ...utils.database import get_db_connection

def get_filtered_jobs(
    search: str = "",
    category_id: Optional[int] = None,
    class_type: Optional[str] = None,
    show_prerequisites: bool = False
) -> List[Dict]:
    """Get filtered list of job classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Build query conditions
        conditions = ["c.is_racial = FALSE"]
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

def render_job_editor():
    """Render the job class editor interface"""
    st.header("Job Class Editor")

    # Initialize session state
    if 'selected_job_id' not in st.session_state:
        st.session_state.selected_job_id = None

    # Load available job classes
    col1, col2 = st.columns([1, 3])
    
    with col1:
        st.subheader("Available Jobs")
        
        # Search box
        search = st.text_input("Search Jobs", key="job_search")
        
        # Category filter
        categories = [(None, "All Categories")]
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = FALSE 
            ORDER BY name
        """)
        categories.extend(cursor.fetchall())
        conn.close()
        
        category_id = st.selectbox(
            "Category",
            options=[c[0] for c in categories],
            format_func=lambda x: next(c[1] for c in categories if c[0] == x),
            key="job_category_filter"
        )

        # Class type filter
        class_types = [(None, "All Types")]
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT name FROM class_types ORDER BY name")
        class_types.extend((name[0], name[0]) for name in cursor.fetchall())
        conn.close()

        class_type = st.selectbox(
            "Class Type",
            options=[t[0] for t in class_types],
            format_func=lambda x: next(t[1] for t in class_types if t[0] == x),
            key="job_type_filter"
        )

        # Additional filters
        show_prerequisites = st.checkbox("Has Prerequisites", key="show_prerequisites")

        # Get filtered jobs
        jobs = get_filtered_jobs(
            search=search,
            category_id=category_id,
            class_type=class_type,
            show_prerequisites=show_prerequisites
        )
        
        # Display jobs list
        if jobs:
            for job in jobs:
                if st.button(
                    f"{job['name']} ({job['category_name']})",
                    key=f"job_{job['id']}",
                    use_container_width=True,
                    type="secondary" if job['id'] != st.session_state.selected_job_id else "primary"
                ):
                    st.session_state.selected_job_id = job['id']
                    # Clear editor state
                    if 'prereq_groups' in st.session_state:
                        del st.session_state.prereq_groups
                    if 'exclusions' in st.session_state:
                        del st.session_state.exclusions
                    st.rerun()
        else:
            st.write("No jobs found matching filters")

        # New job button
        if st.button("Create New Job", use_container_width=True):
            st.session_state.selected_job_id = None
            # Clear editor state
            if 'prereq_groups' in st.session_state:
                del st.session_state.prereq_groups
            if 'exclusions' in st.session_state:
                del st.session_state.exclusions
            st.rerun()

    # Editor section
    with col2:
        # Extra fields specific to job classes could be added here
        extra_fields = []
        
        # Render shared class editor
        render_class_editor(
            class_id=st.session_state.selected_job_id,
            is_racial=False,
            extra_fields=extra_fields,
            mode_prefix="job_"
        )