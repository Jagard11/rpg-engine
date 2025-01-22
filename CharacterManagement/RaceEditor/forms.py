# ./CharacterManagement/RaceEditor/forms.py

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

def render_prerequisites_tab():
    """Render the prerequisites tab (placeholder)"""
    st.info("Prerequisites feature coming soon...")
    return {}

def render_exceptions_tab():
    """Render the exceptions tab (placeholder)"""
    st.info("Exceptions feature coming soon...")
    return {}

def render_spell_list_tab():
    """Render the spell list tab (placeholder)"""
    st.info("Spell list feature coming soon...")
    return {}

def render_appearance_tab():
    """Render the appearance tab (placeholder)"""
    st.info("Appearance customization feature coming soon...")
    return {}

def render_race_form(race_data: Optional[Dict] = None) -> Dict:
    """Render the race editor form with tabs"""
    
    with st.form("race_editor_form"):
        # Create tabs
        tabs = st.tabs([
            "Basic Information",
            "Base Stats",
            "Stats Per Level",
            "Prerequisites",
            "Exceptions",
            "Spell List",
            "Appearance"
        ])
        
        # Render each tab's content
        with tabs[0]:
            basic_info = render_basic_info_tab(race_data)
            
        with tabs[1]:
            base_stats = render_base_stats_tab(race_data)
            
        with tabs[2]:
            stats_per_level = render_stats_per_level_tab(race_data)
            
        with tabs[3]:
            prerequisites = render_prerequisites_tab()
            
        with tabs[4]:
            exceptions = render_exceptions_tab()
            
        with tabs[5]:
            spell_list = render_spell_list_tab()
            
        with tabs[6]:
            appearance = render_appearance_tab()
        
        # Submit button
        submitted = st.form_submit_button("Save Race")
        
        if submitted:
            # Combine all the data from tabs
            return {
                'id': race_data.get('id') if race_data else None,
                **basic_info,
                **base_stats,
                **stats_per_level,
                **prerequisites,
                **exceptions,
                **spell_list,
                **appearance
            }
        
        return None