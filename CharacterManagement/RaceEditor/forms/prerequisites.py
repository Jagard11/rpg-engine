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
        conn.close()

def render_prerequisites_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the prerequisites tab"""
    
    if not race_data or not race_data.get('id'):
        st.info("Save the race first to add prerequisites.")
        return {}
        
    # Get existing prerequisites
    prerequisites = get_prerequisites(race_data['id'])
    
    # Group prerequisites by prerequisite_group
    prereq_groups = {}
    for prereq in prerequisites:
        group = prereq['prerequisite_group']
        if group not in prereq_groups:
            prereq_groups[group] = []
        prereq_groups[group].append(prereq)
    
    # Initialize session state for prerequisites if needed
    if 'prerequisites' not in st.session_state:
        st.session_state.prerequisites = prerequisites
    
    # Button to add new prerequisite group
    if st.button("Add Prerequisite Group"):
        new_group = max(prereq_groups.keys(), default=0) + 1
        st.session_state.prerequisites.append({
            'prerequisite_group': new_group,
            'prerequisite_type': 'specific_class',
            'target_id': None,
            'required_level': None,
            'min_value': None,
            'max_value': None
        })
        prereq_groups[new_group] = [st.session_state.prerequisites[-1]]
        st.rerun()
    
    # Render each prerequisite group
    for group_num, group_prereqs in sorted(prereq_groups.items()):
        with st.expander(f"Prerequisite Group {group_num} (OR)", expanded=True):
            st.write("All prerequisites in this group must be met (AND)")
            
            # Button to add prerequisite to group
            if st.button(f"Add Prerequisite to Group {group_num}"):
                st.session_state.prerequisites.append({
                    'prerequisite_group': group_num,
                    'prerequisite_type': 'specific_class',
                    'target_id': None,
                    'required_level': None,
                    'min_value': None,
                    'max_value': None
                })
                st.rerun()
            
            # Render each prerequisite in the group
            for i, prereq in enumerate(group_prereqs):
                with st.container():
                    col1, col2, col3, col4 = st.columns([3, 2, 2, 1])
                    
                    with col1:
                        prereq_type = st.selectbox(
                            "Type",
                            options=['specific_class', 'category_total', 
                                   'subcategory_total', 'karma', 'quest', 
                                   'achievement'],
                            key=f"type_{group_num}_{i}",
                            index=['specific_class', 'category_total', 
                                  'subcategory_total', 'karma', 'quest', 
                                  'achievement'].index(prereq['prerequisite_type'])
                        )
                    
                    with col2:
                        # Show appropriate selector based on prerequisite type
                        if prereq_type in ['specific_class']:
                            # Get all classes
                            cursor = sqlite3.connect('rpg_data.db').cursor()
                            cursor.execute("SELECT id, name FROM classes ORDER BY name")
                            classes = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
                            
                            target_index = find_index_by_id(classes, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                "Class",
                                options=[c["id"] for c in classes],
                                format_func=lambda x: next(c["name"] for c in classes if c["id"] == x),
                                key=f"target_{group_num}_{i}",
                                index=target_index
                            )
                            
                        elif prereq_type in ['category_total']:
                            categories = get_race_categories()
                            target_index = find_index_by_id(categories, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                "Category",
                                options=[cat["id"] for cat in categories],
                                format_func=lambda x: next(cat["name"] for cat in categories if cat["id"] == x),
                                key=f"target_{group_num}_{i}",
                                index=target_index
                            )
                            
                        elif prereq_type in ['subcategory_total']:
                            subcategories = get_subcategories()
                            target_index = find_index_by_id(subcategories, prereq.get('target_id', 0))
                            prereq['target_id'] = st.selectbox(
                                "Subcategory",
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
    
    # Save button for prerequisites
    if st.button("Save Prerequisites"):
        try:
            save_prerequisites(race_data['id'], st.session_state.prerequisites)
            st.success("Prerequisites saved successfully!")
        except Exception as e:
            st.error(f"Error saving prerequisites: {str(e)}")
    
    return {'prerequisites': st.session_state.prerequisites}