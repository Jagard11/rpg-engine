# ./CharacterManagement/RaceEditor/forms/baseInfo.py

import streamlit as st
from typing import Dict, List, Optional
import sqlite3

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