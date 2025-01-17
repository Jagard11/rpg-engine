# ./main.py

import streamlit as st
import sqlite3
import os
from pathlib import Path
from ServerMessage import render_server_tab
from CharacterInfo import init_character_state, render_character_tab
from DatabaseInspector import render_db_inspector_tab
from DatabaseSetup.createCharacterSchema import create_character_schema
from DatabaseSetup.createSpellSchema import create_spell_schema
from DatabaseSetup.createClassSystemSchema import ClassSystemInitializer

# Must be the first Streamlit command
st.set_page_config(page_title="RPG Communication Interface", layout="wide")

def init_database():
    """Initialize database and create tables if they don't exist"""
    try:
        # Check if database exists
        db_path = Path('rpg_data.db')
        if not db_path.exists():
            print("Database not found. Creating new database...")
            
            # Initialize all schemas
            create_character_schema()
            create_spell_schema()
            
            # Initialize class system
            class_initializer = ClassSystemInitializer()
            class_initializer.setup_class_system()
            
            print("Database initialization complete.")
        
    except Exception as e:
        st.error(f"Error initializing database: {str(e)}")
        raise

# Initialize database before any other operations
init_database()

# Initialize character state
init_character_state()

# Main app with tabs
tab1, tab2, tab3 = st.tabs(["Server Messages", "Character", "Database Inspector"])

with tab1:
    render_server_tab()
    
with tab2:
    render_character_tab()

with tab3:
    render_db_inspector_tab()