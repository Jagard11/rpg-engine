# ./main.py

import streamlit as st
from pathlib import Path

# Import character management module
from CharacterManagement import (
    render_character_editor,
    render_job_editor, 
    render_race_editor
)
from ClassManager import render_class_manager
from ServerMessage import render_server_tab
from LocationManager import render_location_editor_tab
from DatabaseInspector import render_db_inspector_tab

def init_database():
    """Initialize database connection and required tables"""
    db_path = Path('rpg_data.db')
    if not db_path.exists():
        st.error("Database not found. Please run SchemaManager/initializeSchema.py first.")
        raise FileNotFoundError("Database not found")

# Initialize application
st.set_page_config(page_title="RPG Character Management", layout="wide")
init_database()

# Create tabs
tab1, tab2, tab3, tab4, tab5 = st.tabs([
    "Character Editors", 
    "Class Editor",
    "Location Editor", 
    "Database Editor", 
    "Server Message"
])

with tab1:
    # Create subtabs for different editors
    char_tab, job_tab, race_tab = st.tabs([
        "Character Editor",
        "Job Editor", 
        "Race Editor"
    ])
    
    with char_tab:
        render_character_editor()
        
    with job_tab:
        render_job_editor()
        
    with race_tab:
        render_race_editor()

with tab2:
    class_list_tab, class_editor_tab, class_spell_tab = st.tabs([
        "Class List",
        "Class Editor",
        "Class Spell List"
    ])
    
    with class_list_tab:
        render_class_manager("list")
        
    with class_editor_tab:
        render_class_manager("editor")
        
    with class_spell_tab:
        render_class_manager("spells")

with tab3:
    render_location_editor_tab()

with tab4:
    render_db_inspector_tab()
    
with tab5:
    render_server_tab()