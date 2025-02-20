# ./CharacterManagement/CharacterEditor/forms.py

import sqlite3
import streamlit as st
from typing import Dict, List, Optional

def get_class_types() -> List[Dict]:
    """Get available class types from database"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_types 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        cursor.close()
        conn.close()

def get_race_categories() -> List[Dict]:
    """Get list of racial class categories"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = TRUE
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        cursor.close()
        conn.close()

def get_subcategories() -> List[Dict]:
    """Get available subcategories from database"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_subcategories 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        cursor.close()
        conn.close()

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

def find_index_by_id(items: List[Dict], target_id: int, default: int = 0) -> int:
    """Helper function to find index of item by id"""
    for i, item in enumerate(items):
        if item['id'] == target_id:
            return i
    return default

def render_basic_info_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the basic information tab"""
    col1, col2 = st.columns(2)
    
    with col1:
        name = st.text_input(
            "Name",
            value=race_data.get('name', '') if race_data else ''
        )
        
        # Class Type Selection
        class_types = get_class_types()
        class_type_index = find_index_by_id(
            class_types, 
            race_data.get('class_type', 0) if race_data else 0
        )
        
        class_type = st.selectbox(
            "Class Type",
            options=[t["id"] for t in class_types],
            format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
            index=class_type_index
        )
    
    with col2:
        categories = get_race_categories()
        category_index = find_index_by_id(
            categories, 
            race_data.get('category_id', 0) if race_data else 0
        )
        
        category_id = st.selectbox(
            "Category",
            options=[cat["id"] for cat in categories],
            format_func=lambda x: next(cat["name"] for cat in categories if cat["id"] == x),
            index=category_index
        )
        
        # Subcategory Selection
        subcategories = get_subcategories()
        subcategory_index = find_index_by_id(
            subcategories, 
            race_data.get('subcategory_id', 0) if race_data else 0
        )
        
        subcategory_id = st.selectbox(
            "Subcategory",
            options=[sub["id"] for sub in subcategories],
            format_func=lambda x: next(sub["name"] for sub in subcategories if sub["id"] == x),
            index=subcategory_index
        )
    
    # Description
    description = st.text_area(
        "Description",
        value=race_data.get('description', '') if race_data else ''
    )
    
    return {
        'name': name,
        'class_type': class_type,
        'category_id': category_id,
        'subcategory_id': subcategory_id,
        'description': description
    }

def render_base_stats_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the base stats tab"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        base_hp = st.number_input(
            "Base HP",
            value=float(race_data.get('base_hp', 0)) if race_data else 0.0
        )
        base_mp = st.number_input(
            "Base MP",
            value=float(race_data.get('base_mp', 0)) if race_data else 0.0
        )
        base_physical_attack = st.number_input(
            "Base Physical Attack",
            value=float(race_data.get('base_physical_attack', 0)) if race_data else 0.0
        )
    
    with col2:
        base_physical_defense = st.number_input(
            "Base Physical Defense",
            value=float(race_data.get('base_physical_defense', 0)) if race_data else 0.0
        )
        base_magical_attack = st.number_input(
            "Base Magical Attack",
            value=float(race_data.get('base_magical_attack', 0)) if race_data else 0.0
        )
        base_magical_defense = st.number_input(
            "Base Magical Defense",
            value=float(race_data.get('base_magical_defense', 0)) if race_data else 0.0
        )
    
    with col3:
        base_agility = st.number_input(
            "Base Agility",
            value=float(race_data.get('base_agility', 0)) if race_data else 0.0
        )
        base_resistance = st.number_input(
            "Base Resistance",
            value=float(race_data.get('base_resistance', 0)) if race_data else 0.0
        )
        base_special = st.number_input(
            "Base Special",
            value=float(race_data.get('base_special', 0)) if race_data else 0.0
        )
    
    return {
        'base_hp': base_hp,
        'base_mp': base_mp,
        'base_physical_attack': base_physical_attack,
        'base_physical_defense': base_physical_defense,
        'base_magical_attack': base_magical_attack,
        'base_magical_defense': base_magical_defense,
        'base_agility': base_agility,
        'base_resistance': base_resistance,
        'base_special': base_special
    }

def render_stats_per_level_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the stats per level tab"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        hp_per_level = st.number_input(
            "HP per Level",
            value=float(race_data.get('hp_per_level', 0)) if race_data else 0.0
        )
        mp_per_level = st.number_input(
            "MP per Level",
            value=float(race_data.get('mp_per_level', 0)) if race_data else 0.0
        )
        physical_attack_per_level = st.number_input(
            "Physical Attack per Level",
            value=float(race_data.get('physical_attack_per_level', 0)) if race_data else 0.0
        )
    
    with col2:
        physical_defense_per_level = st.number_input(
            "Physical Defense per Level",
            value=float(race_data.get('physical_defense_per_level', 0)) if race_data else 0.0
        )
        magical_attack_per_level = st.number_input(
            "Magical Attack per Level",
            value=float(race_data.get('magical_attack_per_level', 0)) if race_data else 0.0
        )
        magical_defense_per_level = st.number_input(
            "Magical Defense per Level",
            value=float(race_data.get('magical_defense_per_level', 0)) if race_data else 0.0
        )
    
    with col3:
        agility_per_level = st.number_input(
            "Agility per Level",
            value=float(race_data.get('agility_per_level', 0)) if race_data else 0.0
        )
        resistance_per_level = st.number_input(
            "Resistance per Level",
            value=float(race_data.get('resistance_per_level', 0)) if race_data else 0.0
        )
        special_per_level = st.number_input(
            "Special per Level",
            value=float(race_data.get('special_per_level', 0)) if race_data else 0.0
        )
    
    return {
        'hp_per_level': hp_per_level,
        'mp_per_level': mp_per_level,
        'physical_attack_per_level': physical_attack_per_level,
        'physical_defense_per_level': physical_defense_per_level,
        'magical_attack_per_level': magical_attack_per_level,
        'magical_defense_per_level': magical_defense_per_level,
        'agility_per_level': agility_per_level,
        'resistance_per_level': resistance_per_level,
        'special_per_level': special_per_level
    }

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

def render_race_form(race_data: Optional[Dict] = None) -> None:
    """Render the complete race form with tabs"""
    
    # Create tabs
    tab1, tab2, tab3, tab4 = st.tabs([
        "Basic Information",
        "Base Stats",
        "Stats per Level",
        "Prerequisites"
    ])
    
    # Render each tab
    with tab1:
        basic_info = render_basic_info_tab(race_data)
    
    with tab2:
        base_stats = render_base_stats_tab(race_data)
    
    with tab3:
        stats_per_level = render_stats_per_level_tab(race_data)
    
    with tab4:
        prerequisites = render_prerequisites_tab(race_data)
    
    # Main save button for race data
    if st.button("Save Race"):
        if not basic_info.get('name'):
            st.error("Name is required!")
            return
            
        # Combine all data
        save_data = {
            'id': race_data.get('id') if race_data else None,
            **basic_info,
            **base_stats,
            **stats_per_level
        }
        
        try:
            # Save race data to database
            conn = sqlite3.connect('rpg_data.db')
            cursor = conn.cursor()
            
            if save_data.get('id'):
                # Update existing race
                fields = [f"{k} = ?" for k in save_data.keys() if k != 'id']
                query = f"""
                    UPDATE races 
                    SET {', '.join(fields)}
                    WHERE id = ?
                """
                values = [v for k, v in save_data.items() if k != 'id']
                values.append(save_data['id'])
                
                cursor.execute(query, values)
            else:
                # Insert new race
                fields = [k for k in save_data.keys() if k != 'id']
                placeholders = ['?' for _ in fields]
                query = f"""
                    INSERT INTO races ({', '.join(fields)})
                    VALUES ({', '.join(placeholders)})
                """
                values = [save_data[k] for k in fields]
                
                cursor.execute(query, values)
                save_data['id'] = cursor.lastrowid
            
            conn.commit()
            st.success(f"Race {'updated' if race_data else 'created'} successfully!")
            
            # Refresh the page to show updated data
            st.experimental_rerun()
            
        except Exception as e:
            st.error(f"Error saving race: {str(e)}")
        finally:
            conn.close()