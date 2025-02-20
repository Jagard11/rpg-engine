# ./main.py

import streamlit as st
from pathlib import Path
from CharacterManagement import (
    render_character_editor,
    render_job_editor,
    render_race_editor
)
from ClassManager import render_class_manager
from ServerMessage import render_server_tab
from LocationManager import render_location_editor_tab
from DatabaseInspector import render_db_inspector_tab
from SpellEffectManager import render_spell_effect_editor

def init_database():
    """Initialize database connection and required tables"""
    db_path = Path('rpg_data.db')
    if not db_path.exists():
        st.error("Database not found. Please run SchemaManager/initializeSchema.py first.")
        raise FileNotFoundError("Database not found")

st.set_page_config(page_title="RPG Character Management", layout="wide")
init_database()

# Editor definitions with script keys
editors = {
    "character": render_character_editor,
    "job": render_job_editor,
    "race": render_race_editor,
    "class": lambda: render_class_manager("editor"),
    "location": render_location_editor_tab,
    "spell_effect": render_spell_effect_editor,
    "db_inspector": render_db_inspector_tab,
    "server": render_server_tab
}

# Display names and URLs for hyperlinks
editor_links = {
    "character": "Character Editor",
    "job": "Job Editor",
    "race": "Race Editor",
    "class": "Class Manager",
    "location": "Location Editor",
    "spell_effect": "Spell Effect Editor",
    "db_inspector": "Database Inspector",
    "server": "Server Message"
}

# Query parameter handling
query_params = st.query_params
script_to_run = query_params.get("script", "main") if "script" in query_params else "main"

if script_to_run == "main":
    st.title("RPG Editor Suite")
    st.write("Click an editor link below to open it in a new tab. Right-click and 'Open in New Tab' to edit multiple records simultaneously.")

    # Organized hyperlink list
    col1, col2, col3 = st.columns(3)
    editor_items = list(editor_links.items())
    
    with col1:
        for script, name in editor_items[:3]:  # First 3 editors
            editor_url = f"http://localhost:8501/?script={script}"
            st.markdown(f"<a href='{editor_url}' target='_blank'>{name}</a>", unsafe_allow_html=True)
    
    with col2:
        for script, name in editor_items[3:6]:  # Next 3 editors
            editor_url = f"http://localhost:8501/?script={script}"
            st.markdown(f"<a href='{editor_url}' target='_blank'>{name}</a>", unsafe_allow_html=True)
    
    with col3:
        for script, name in editor_items[6:]:  # Remaining editors
            editor_url = f"http://localhost:8501/?script={script}"
            st.markdown(f"<a href='{editor_url}' target='_blank'>{name}</a>", unsafe_allow_html=True)

    st.info("Tip: Use your browser's tab functionality to open multiple editors at once.")
else:
    if script_to_run in editors:
        editors[script_to_run]()
    else:
        st.error(f"Unknown editor: {script_to_run}")