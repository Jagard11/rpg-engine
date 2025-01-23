# ./CharacterManagement/RaceEditor/forms/raceSelect.py

import streamlit as st
import sqlite3
from typing import Dict, List, Optional
import pandas as pd

def get_filtered_races(
    search_term: str = "",
    class_type_id: Optional[int] = None,
    category_id: Optional[int] = None,
    subcategory_id: Optional[int] = None,
    prerequisites: List[Dict] = None
) -> List[Dict]:
    """Get filtered list of races based on criteria"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        conditions = ["c.is_racial = TRUE"]
        params = []

        # Basic filters
        if search_term:
            conditions.append("(c.name LIKE ? OR c.description LIKE ?)")
            search = f"%{search_term}%"
            params.extend([search, search])
            
        if class_type_id is not None:
            conditions.append("c.class_type = ?")
            params.append(class_type_id)
            
        if category_id is not None:
            conditions.append("c.category_id = ?")
            params.append(category_id)
            
        if subcategory_id is not None:
            conditions.append("c.subcategory_id = ?")
            params.append(subcategory_id)

        # Build prerequisite conditions if any
        if prerequisites:
            prereq_conditions = []
            for prereq in prerequisites:
                if prereq['type'] == 'level_range':
                    if prereq['min_level']:
                        conditions.append("c.max_level >= ?")
                        params.append(prereq['min_level'])
                    if prereq['max_level']:
                        conditions.append("c.min_level <= ?")
                        params.append(prereq['max_level'])
                elif prereq['type'] == 'karma_range':
                    if prereq['min_karma']:
                        conditions.append("EXISTS (SELECT 1 FROM class_prerequisites cp WHERE cp.class_id = c.id AND cp.prerequisite_type = 'karma' AND cp.min_value >= ?)")
                        params.append(prereq['min_karma'])
                    if prereq['max_karma']:
                        conditions.append("EXISTS (SELECT 1 FROM class_prerequisites cp WHERE cp.class_id = c.id AND cp.prerequisite_type = 'karma' AND cp.max_value <= ?)")
                        params.append(prereq['max_karma'])

        # Build and execute query
        query = f"""
            SELECT 
                c.id,
                c.name,
                c.description,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name,
                c.base_hp,
                c.base_mp,
                c.base_physical_attack,
                c.base_magical_attack,
                c.base_agility,
                c.base_resistance,
                c.base_special
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE {' AND '.join(conditions)}
            ORDER BY cc.name, c.name
        """
        
        cursor.execute(query, params)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
        
    finally:
        conn.close()

def render_prerequisite_filters() -> List[Dict]:
    """Render filters for prerequisites"""
    prerequisites = []
    
    with st.expander("Prerequisite Filters", expanded=False):
        # Level range filter
        col1, col2 = st.columns(2)
        with col1:
            min_level = st.number_input("Minimum Level", min_value=0, value=0, key="race_list_min_level")
        with col2:
            max_level = st.number_input("Maximum Level", min_value=0, value=100, key="race_list_max_level")
            
        if min_level > 0 or max_level < 100:
            prerequisites.append({
                'type': 'level_range',
                'min_level': min_level,
                'max_level': max_level
            })
            
        # Karma range filter
        col1, col2 = st.columns(2)
        with col1:
            min_karma = st.number_input("Minimum Karma", value=0, key="race_list_min_karma")
        with col2:
            max_karma = st.number_input("Maximum Karma", value=0, key="race_list_max_karma")
            
        if min_karma != 0 or max_karma != 0:
            prerequisites.append({
                'type': 'karma_range',
                'min_karma': min_karma,
                'max_karma': max_karma
            })
    
    return prerequisites

def render_race_table(races: List[Dict]) -> None:
    """Render races in a table format"""
    if not races:
        st.info("No races found matching the filters")
        return
        
    # Convert to DataFrame for easier display
    df = pd.DataFrame(races)
    
    # Reorder and rename columns for display
    display_columns = {
        'name': 'Name',
        'category_name': 'Category',
        'subcategory_name': 'Subcategory',
        'type_name': 'Type',
        'base_hp': 'HP',
        'base_mp': 'MP',
        'base_physical_attack': 'P.ATK',
        'base_magical_attack': 'M.ATK',
        'base_agility': 'AGI',
        'base_resistance': 'RES',
        'base_special': 'SP'
    }
    
    df_display = df[display_columns.keys()].rename(columns=display_columns)
    
    # Format numeric columns
    numeric_cols = ['HP', 'MP', 'P.ATK', 'M.ATK', 'AGI', 'RES', 'SP']
    for col in numeric_cols:
        df_display[col] = df_display[col].round(1)
    
    # Display table with sorting enabled
    st.dataframe(
        df_display,
        column_config={
            "Name": st.column_config.TextColumn(width="medium"),
            "Category": st.column_config.TextColumn(width="small"),
            "Subcategory": st.column_config.TextColumn(width="small"),
            "Type": st.column_config.TextColumn(width="small"),
        },
        hide_index=True
    )

def render_race_select_tab(class_types: List[Dict], categories: List[Dict], subcategories: List[Dict]) -> None:
    """Render the race select tab with filters and table"""
    
    # Basic filters
    col1, col2, col3, col4 = st.columns(4)
    
    with col1:
        search_term = st.text_input("Search", key="race_table_search")
        
    with col2:
        class_type = st.selectbox(
            "Class Type",
            options=[None] + [t["id"] for t in class_types],
            format_func=lambda x: "All Types" if x is None else next(t["name"] for t in class_types if t["id"] == x),
            key="race_table_type_filter"
        )
        
    with col3:
        category = st.selectbox(
            "Category",
            options=[None] + [c["id"] for c in categories],
            format_func=lambda x: "All Categories" if x is None else next(c["name"] for c in categories if c["id"] == x),
            key="race_table_category_filter"
        )
        
    with col4:
        subcategory = st.selectbox(
            "Subcategory",
            options=[None] + [s["id"] for s in subcategories],
            format_func=lambda x: "All Subcategories" if x is None else next(s["name"] for s in subcategories if s["id"] == x),
            key="race_table_subcategory_filter"
        )
    
    # Prerequisite filters
    prerequisites = render_prerequisite_filters()
    
    # Get and display filtered races
    races = get_filtered_races(
        search_term=search_term,
        class_type_id=class_type,
        category_id=category,
        subcategory_id=subcategory,
        prerequisites=prerequisites
    )
    
    # Display results
    render_race_table(races)