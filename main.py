# ./main.py

import streamlit as st
from pathlib import Path

# Import character management module
from CharacterManagement import (
    render_character_editor,
    render_job_editor, 
    render_race_editor
)
from ServerMessage import render_server_tab
from DatabaseInspector import render_db_inspector_tab

def init_database():
    """Initialize database connection"""
    db_path = Path('rpg_data.db')
    if not db_path.exists():
        st.error("Database not found. Please run SchemaManager/initializeSchema.py first.")
        raise FileNotFoundError("Database not found")

# Initialize application
st.set_page_config(page_title="RPG Character Management", layout="wide")
init_database()

# Create tabs
tab1, tab2, tab3 = st.tabs(["Character Editors", "Server Message", "Database Editor"])

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
    render_server_tab()

with tab3:
    render_db_inspector_tab()