# ./FrontEnd/CharacterInfo/CharacterInfo.py

import streamlit as st
import sqlite3
from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple
from datetime import datetime
from .models.Character import Character
from .utils.database import get_db_connection
from .views.CharacterView import render_character_view
from .views.CharacterCreation import render_character_creation_form
from .views.RaceCreation import render_race_creation_form
from .views.LevelUp import render_level_up_tab
from .views.JobClassesTab import render_job_classes_tab

def init_character_state():
    """Initialize character-related session state variables"""
    if 'current_character' not in st.session_state:
        st.session_state.current_character = None
    if 'character_list' not in st.session_state:
        st.session_state.character_list = []
        load_character_list()
    if 'selected_class_type' not in st.session_state:
        st.session_state.selected_class_type = "All"
    if 'selected_category' not in st.session_state:
        st.session_state.selected_category = "All"
    if 'show_racial' not in st.session_state:
        st.session_state.show_racial = False
    if 'show_existing' not in st.session_state:
        st.session_state.show_existing = True

def load_character_list():
    """Load list of available characters from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id, 
                c.first_name,
                COALESCE(c.last_name, ''),
                c.total_level,
                c.talent,
                cc.name as race_category
            FROM characters c
            JOIN class_categories cc ON c.race_category_id = cc.id
            WHERE c.is_active = TRUE
            ORDER BY c.first_name, c.last_name
        """)
        st.session_state.character_list = cursor.fetchall()
    except Exception as e:
        st.error(f"Error loading character list: {str(e)}")
    finally:
        conn.close()

def render_character_tab():
    """Display character information"""
    st.header("Character Management")
    
    # Tabs for different character operations
    tab1, tab2, tab3, tab4, tab5 = st.tabs([
        "View Characters",
        "Create Character",
        "Create Race",
        "Level Up",
        "Job Classes"
    ])
    
    with tab1:
        if st.session_state.character_list:
            render_character_view()
        else:
            st.info("No characters found. Create one in the 'Create Character' tab!")
            
    with tab2:
        render_character_creation_form()
        
    with tab3:
        render_race_creation_form()

    with tab4:
        if st.session_state.character_list:
            render_level_up_tab()
        else:
            st.info("No characters found. Create one in the 'Create Character' tab!")

    with tab5:
        render_job_classes_tab()