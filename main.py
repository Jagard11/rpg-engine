# ./main.py

import streamlit as st
import sqlite3
import os
from pathlib import Path
from ServerMessage import render_server_tab
from CharacterInfo import init_character_state, render_character_tab
from DatabaseInspector import render_db_inspector_tab

# Must be the first Streamlit command
st.set_page_config(page_title="RPG Communication Interface", layout="wide")

def init_database():
    """Initialize database if it doesn't exist"""
    try:
        # Check if database exists
        db_path = Path('rpg_data.db')
        if not db_path.exists():
            st.error("Database not found. Please run DatabaseSetup/setupDatabase.py to create the initial database.")
            raise FileNotFoundError("Database not found")
            
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