# ./CharacterManagement/RaceEditor/forms/prerequisites.py

import streamlit as st
import sqlite3
from typing import Dict, List, Optional
from .baseInfo import get_race_categories, get_subcategories, find_index_by_id

def get_prerequisites(race_id: int) -> List[Dict]:
    """Get prerequisites for a specific race"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                id,
                prerequisite_group,
                prerequisite_type,
                target_id,
                required_level,
                min_value,
                max_value
            FROM class_prerequisites
            WHERE class_id = ?
            ORDER BY prerequisite_group, id
        """, (race_id,))
        
        columns = ['id', 'prerequisite_group', 'prerequisite_type', 
                  'target_id', 'required_level', 'min_value', 'max_value']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        cursor.close()
        conn.close()

def save_prerequisites(race_id: int, prerequisites: List[Dict]) -> None:
    """Save prerequisites for a race
    
    Args:
        race_id: ID of the race
        prerequisites: List of prerequisite dictionaries
    """
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        # Start transaction
        cursor.execute("BEGIN TRANSACTION")
        
        # Delete existing prerequisites
        cursor.execute("""
            DELETE FROM class_prerequisites 
            WHERE class_id = ?
        """, (race_id,))
        
        # Insert new prerequisites
        for prereq in prerequisites:
            cursor.execute("""
                INSERT INTO class_prerequisites (
                    class_id,
                    prerequisite_group,
                    prerequisite_type,
                    target_id,
                    required_level,
                    min_value,
                    max_value
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (
                race_id,
                prereq['prerequisite_group'],
                prereq['prerequisite_type'],
                prereq.get('target_id'),
                prereq.get('required_level'),
                prereq.get('min_value'),
                prereq.get('max_value')
            ))
        
        # Commit transaction
        cursor.execute("COMMIT")
    except Exception as e:
        cursor.execute("ROLLBACK")
        raise e
    finally:
        cursor.close()
        conn.close()

def render_prerequisites_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the prerequisites tab"""
    
    if not race_data or not race_data.get('id'):
        st.info("Save the race first to add prerequisites.")
        return {}
        
    # Get existing prerequisites
    prerequisites = get_prerequisites(race_data['id'])
    
    # Initialize or update session state for prerequisites
    if 'prerequisites' not in st.session_state or st.session_state.get('race_id') != race_data['id']:
        st.session_state.prerequisites = prerequisites
        st.session_state.race_id = race_data['id']
    
    # Group prerequisites by prerequisite_group
    prereq_groups = {}
    for prereq in st.session_state.prerequisites:
        group = prereq['prerequisite_group']
        if group not in prereq_groups:
            prereq_groups[group] = []
        prereq_groups[group].append(prereq)
    
    # Initialize prerequisites list in session state if not present
    if 'prerequisites' not in st.session_state:
        st.session_state.prerequisites = []
        
    # Button to add new prerequisite group
    if st.button("Add Prerequisite Group"):
        # Get the new group number (either 1 or max + 1)
        new_group = max([0] + [p['prerequisite_group'] for p in st.session_state.prerequisites]) + 1
        
        # Create a new prerequisite record
        new_prerequisite = {
            'prerequisite_group': new_group,
            'prerequisite_type': 'specific_race',  # Default to specific race type
            'target_id': None,
            'required_level': 0,
            'min_value': None,
            'max_value': None
        }
        
        # Add to session state
        st.session_state.prerequisites.append(new_prerequisite)
        st.rerun()  # Rerun to update the display
    
    # Render each prerequisite group
    for group_num, group_prereqs in sorted(prereq_groups.items()):
        with st.expander(f"Prerequisite Group {group_num} (OR)", expanded=True):
            st.write("All prerequisites in this group must be met (AND)")
            
            # Button to add prerequisite to group
            if st.button(f"Add Prerequisite to Group {group_num}", key=f"add_prereq_{group_num}"):
                # Create a new prerequisite in the same group
                new_prerequisite = {
                    'prerequisite_group': group_num,
                    'prerequisite_type': 'specific_race',  # Default to specific race type
                    'target_id': None,
                    'required_level': 0,
                    'min_value': None,
                    'max_value': None
                }
                
                # Add to session state
                st.session_state.prerequisites.append(new_prerequisite)
                st.rerun()  # Rerun to update the display
            
            # Render each prerequisite in the group
            for i, prereq in enumerate(group_prereqs):
                with st.container():
                    col1, col2, col3, col4 = st.columns([3, 2, 2, 1])
                    
                    with col1:
                        prereq_type = st.selectbox(
                            "Type",
                            options=[
                                # Race-related prerequisites
                                'specific_race', 'race_category_total', 'race_subcategory_total',
                                # Job-related prerequisites
                                'specific_job', 'job_category_total', 'job_subcategory_total',
                                # Other prerequisites
                                'karma', 'quest', 'achievement'
                            ],
                            key=f"type_{group_num}_{i}",
                            index=[
                                'specific_race', 'race_category_total', 'race_subcategory_total',
                                'specific_job', 'job_category_total', 'job_subcategory_total',
                                'karma', 'quest', 'achievement'
                            ].index(prereq.get('prerequisite_type', 'specific_race'))
                        )
                        prereq['prerequisite_type'] = prereq_type
                    
                    with col2:
                        # Show appropriate selector based on prerequisite type
                        if prereq_type in ['specific_race', 'specific_job']:
                            cursor = sqlite3.connect('rpg_data.db').cursor()
                            if prereq_type == 'specific_race':
                                cursor.execute("""
                                    SELECT id, name 
                                    FROM classes 
                                    WHERE is_racial = TRUE 
                                    ORDER BY name
                                """)
                                label = "Race"
                            else:  # specific_job
                                cursor.execute("""
                                    SELECT id, name 
                                    FROM classes 
                                    WHERE is_racial = FALSE 
                                    ORDER BY name
                                """)
                                label = "Job"
                            
                            classes = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
                            cursor.close()
                            
                            target_index = find_index_by_id(classes, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                label,
                                options=[c["id"] for c in classes],
                                format_func=lambda x: next(c["name"] for c in classes if c["id"] == x),
                                key=f"target_{group_num}_{i}",
                                index=target_index
                            )
                            
                        elif prereq_type in ['race_category_total', 'job_category_total']:
                            conn = sqlite3.connect('rpg_data.db')
                            cursor = conn.cursor()
                            
                            if prereq_type == 'race_category_total':
                                cursor.execute("""
                                    SELECT id, name 
                                    FROM class_categories 
                                    WHERE is_racial = TRUE 
                                    ORDER BY name
                                """)
                                label = "Race Category"
                            else:  # job_category_total
                                cursor.execute("""
                                    SELECT id, name 
                                    FROM class_categories 
                                    WHERE is_racial = FALSE 
                                    ORDER BY name
                                """)
                                label = "Job Category"
                            
                            categories = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
                            cursor.close()
                            conn.close()
                            
                            target_index = find_index_by_id(categories, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                label,
                                options=[cat["id"] for cat in categories],
                                format_func=lambda x: next(cat["name"] for cat in categories if cat["id"] == x),
                                key=f"target_{group_num}_{i}",
                                index=target_index
                            )
                            
                        elif prereq_type in ['race_subcategory_total', 'job_subcategory_total']:
                            conn = sqlite3.connect('rpg_data.db')
                            cursor = conn.cursor()
                            
                            if prereq_type == 'race_subcategory_total':
                                cursor.execute("""
                                    SELECT DISTINCT cs.id, cs.name
                                    FROM class_subcategories cs
                                    JOIN classes c ON c.subcategory_id = cs.id
                                    WHERE c.is_racial = TRUE
                                    ORDER BY cs.name
                                """)
                                label = "Race Subcategory"
                            else:  # job_subcategory_total
                                cursor.execute("""
                                    SELECT DISTINCT cs.id, cs.name
                                    FROM class_subcategories cs
                                    JOIN classes c ON c.subcategory_id = cs.id
                                    WHERE c.is_racial = FALSE
                                    ORDER BY cs.name
                                """)
                                label = "Job Subcategory"
                            
                            subcategories = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
                            cursor.close()
                            conn.close()
                            
                            target_index = find_index_by_id(subcategories, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                label,
                                options=[sub["id"] for sub in subcategories],
                                format_func=lambda x: next(sub["name"] for sub in subcategories if sub["id"] == x),
                                key=f"target_{group_num}_{i}",
                                index=target_index
                            )
                            
                    with col3:
                        # Show appropriate value inputs based on prerequisite type
                        if prereq_type in ['specific_class', 'category_total', 'subcategory_total']:
                            prereq['required_level'] = st.number_input(
                                "Required Level",
                                min_value=0,
                                value=prereq.get('required_level', 0),
                                key=f"level_{group_num}_{i}"
                            )
                        elif prereq_type in ['karma']:
                            prereq['min_value'] = st.number_input(
                                "Min Karma",
                                value=prereq.get('min_value', 0),
                                key=f"min_{group_num}_{i}"
                            )
                            prereq['max_value'] = st.number_input(
                                "Max Karma",
                                value=prereq.get('max_value', 0),
                                key=f"max_{group_num}_{i}"
                            )
                        elif prereq_type in ['quest', 'achievement']:
                            prereq['target_id'] = st.number_input(
                                f"{prereq_type.capitalize()} ID",
                                min_value=1,
                                value=prereq.get('target_id', 1),
                                key=f"target_{group_num}_{i}"
                            )
                    
                    with col4:
                        # Delete prerequisite button
                        if st.button("❌", key=f"delete_{group_num}_{i}"):
                            st.session_state.prerequisites.remove(prereq)
                            st.rerun()
    
    return {'prerequisites': st.session_state.prerequisites}