# ./ClassManager/classSpellList.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path
from typing import Optional, Dict, Any

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def get_class_name(class_id: int) -> str:
    """Get the name of a class by its ID"""
    try:
        with get_db_connection() as conn:
            query = "SELECT name FROM classes WHERE id = ?"
            result = conn.execute(query, [class_id]).fetchone()
            return result[0] if result else ""
    except Exception as e:
        st.error(f"Error getting class name: {e}")
        return ""

def search_spells(search_term: str) -> pd.DataFrame:
    """Search the spells table based on name or description"""
    query = """
    SELECT id, name, description, spell_tier, mp_cost
    FROM spells
    WHERE name LIKE ? OR description LIKE ?
    LIMIT 50
    """
    search_pattern = f"%{search_term}%"
    
    try:
        with get_db_connection() as conn:
            return pd.read_sql_query(
                query, 
                conn, 
                params=[search_pattern, search_pattern]
            )
    except Exception as e:
        st.error(f"Error searching spells: {e}")
        return pd.DataFrame()

def get_class_spell_list(class_id: int) -> pd.DataFrame:
    """Get all spells in a class's spell list"""
    query = """
    SELECT 
        csl.id as list_entry_id,
        s.id as spell_id,
        s.name as spell_name,
        s.spell_tier,
        s.mp_cost,
        csl.minimum_level,
        csl.list_name
    FROM class_spell_lists csl
    JOIN spells s ON s.id = csl.spell_id
    WHERE csl.class_id = ?
    ORDER BY csl.minimum_level, s.spell_tier, s.name
    """
    
    try:
        with get_db_connection() as conn:
            return pd.read_sql_query(query, conn, params=[class_id])
    except Exception as e:
        st.error(f"Error getting class spell list: {e}")
        return pd.DataFrame()

def add_spell_to_list(class_id: int, spell_id: int, list_name: str, minimum_level: int) -> bool:
    """Add a spell to the class spell list"""
    query = """
    INSERT INTO class_spell_lists (class_id, spell_id, list_name, minimum_level)
    VALUES (?, ?, ?, ?)
    """
    
    try:
        with get_db_connection() as conn:
            conn.execute(query, [class_id, spell_id, list_name, minimum_level])
            conn.commit()
            return True
    except sqlite3.IntegrityError:
        st.error("This spell is already in the class spell list")
        return False
    except Exception as e:
        st.error(f"Error adding spell to list: {e}")
        return False

def remove_spell_from_list(list_entry_id: int) -> bool:
    """Remove a spell from the class spell list"""
    query = "DELETE FROM class_spell_lists WHERE id = ?"
    
    try:
        with get_db_connection() as conn:
            conn.execute(query, [list_entry_id])
            conn.commit()
            return True
    except Exception as e:
        st.error(f"Error removing spell from list: {e}")
        return False

def render_class_spell_list():
    """Render the class spell list editor interface"""
    st.header("Class Spell List Editor")
    
    # Get current class ID from session state if available
    current_class_id = st.session_state.get('current_class_id', 0)
    if current_class_id == 0:
        st.warning("Please select a class from the Class Editor first")
        return
        
    class_name = get_class_name(current_class_id)
    default_list_name = f"{class_name} Spell List"
    
    # List name input
    list_name = st.text_input(
        "Spell List Name",
        value=default_list_name,
        key="spell_list_name"
    )
    
    # Create two columns for search and current list
    col1, col2 = st.columns(2)
    
    with col1:
        st.subheader("Add Spells")
        # Search interface
        search_term = st.text_input("Search Spells", key="spell_search")
        if search_term:
            results = search_spells(search_term)
            if not results.empty:
                st.dataframe(
                    results[['name', 'spell_tier', 'mp_cost']],
                    hide_index=True,
                    use_container_width=True
                )
                
                # Add spell form
                with st.form("add_spell_form"):
                    selected_spell = st.selectbox(
                        "Select Spell to Add",
                        options=results['id'].tolist(),
                        format_func=lambda x: results[results['id'] == x]['name'].iloc[0]
                    )
                    
                    min_level = st.number_input(
                        "Minimum Level Required",
                        min_value=1,
                        max_value=15,
                        value=1
                    )
                    
                    if st.form_submit_button("Add to List"):
                        if add_spell_to_list(current_class_id, selected_spell, list_name, min_level):
                            st.success("Spell added successfully!")
                            st.rerun()
    
    with col2:
        st.subheader("Current Spell List")
        spell_list = get_class_spell_list(current_class_id)
        if not spell_list.empty:
            for _, spell in spell_list.iterrows():
                with st.container():
                    cols = st.columns([3, 1, 1, 1])
                    with cols[0]:
                        st.write(spell['spell_name'])
                    with cols[1]:
                        st.write(f"Tier: {spell['spell_tier']}")
                    with cols[2]:
                        st.write(f"Min Lvl: {spell['minimum_level']}")
                    with cols[3]:
                        if st.button("Remove", key=f"remove_{spell['list_entry_id']}"):
                            if remove_spell_from_list(spell['list_entry_id']):
                                st.success("Spell removed from list!")
                                st.rerun()