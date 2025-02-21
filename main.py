# ./main.py

import streamlit as st
from pathlib import Path
from CharacterManagement import (
    render_character_editor,
    render_job_editor,
    render_race_editor
)
from CharacterManagement.CharacterEditor.forms.history import render_character_history
from CharacterManagement.CharacterEditor.forms.equipment import render_character_equipment
from CharacterManagement.CharacterEditor.forms.level_distribution import render_level_distribution
from CharacterManagement.CharacterEditor.forms.spell_list import render_spell_list
from CharacterManagement.CharacterEditor.forms.quests import render_completed_quests
from CharacterManagement.CharacterEditor.forms.achievements import render_earned_achievements
from CharacterManagement.RaceEditor.forms.equipment_slots import render_equipment_slots
from CharacterManagement.JobEditor.forms.prerequisites import render_job_prerequisites
from CharacterManagement.JobEditor.forms.conditions import render_job_conditions
from CharacterManagement.JobEditor.forms.spell_list import render_job_spell_list
from ClassManager import render_class_manager
from ClassManager.classesTable import render_job_table
from ServerMessage import render_server_tab
from LocationManager import render_location_editor_tab
from DatabaseInspector import render_db_inspector_tab
from SpellEffectManager import render_spell_effect_editor
from SpellEffectManager.spell_wrappers import render_spell_wrappers

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
    "character_history": render_character_history,
    "character_equipment": render_character_equipment,
    "character_level_distribution": render_level_distribution,
    "character_spell_list": render_spell_list,
    "character_quests": render_completed_quests,
    "character_achievements": render_earned_achievements,
    "job_class_editor": render_job_editor,
    "job_prerequisites": render_job_prerequisites,
    "job_conditions": render_job_conditions,
    "job_spell_list": render_job_spell_list,
    "job_table": render_job_table,
    "race": render_race_editor,
    "race_equipment_slots": render_equipment_slots,
    "class": lambda: render_class_manager("editor"),
    "location": render_location_editor_tab,
    "spell_effect": render_spell_effect_editor,
    "spell_wrappers": render_spell_wrappers,
    "db_inspector": render_db_inspector_tab,
    "server": render_server_tab
}

# Query parameter handling
query_params = st.query_params
script_to_run = query_params.get("script", "main") if "script" in query_params else "main"

if script_to_run == "main":
    st.title("RPG Editor Suite")
    st.write("Click a link below to open an editor in a new tab. Use multiple tabs to edit different records simultaneously.")

    # Define the cells for the grid
    cells = [
        ("Job Editor", [
            ("Job Class Editor", "job_class_editor"),
            ("Job Prerequisites", "job_prerequisites"),
            ("Job Conditions", "job_conditions"),
            ("Job Spell List", "job_spell_list"),
            ("Job Table", "job_table")
        ]),
        ("Race Editor", [
            ("Race Class Editor", "race"),
            ("Race Equipment Slots", "race_equipment_slots")
        ]),
        ("Character Editor", [
            ("Character Editor (Basic Info)", "character"),
            ("Character History", "character_history"),
            ("Equipment", "character_equipment"),
            ("Level Distribution", "character_level_distribution"),
            ("Spell List", "character_spell_list"),
            ("Completed Quests", "character_quests"),
            ("Earned Achievements", "character_achievements")
        ]),
        ("Spell Editor", [
            ("Spell Effect Editor", "spell_effect"),
            ("Spell Wrappers", "spell_wrappers")
        ]),
        ("Location Editor", [
            ("Location Editor", "location")
        ]),
        ("Database Inspector", [
            ("Database Inspector", "db_inspector")
        ]),
        ("Server Message", [
            ("Server Message", "server")
        ])
    ]

    # Generate HTML for the grid with four columns
    html = """
    <style>
    .grid-container {
    display: grid;
    grid-template-columns: repeat(4, 1fr);  /* Sets the grid to 4 columns */
    gap: 10px;
    }
    .grid-cell {
    border: 2px solid white;
    padding: 10px;
    }
    </style>
    <div class="grid-container">
    """
    for title, links in cells:  # Assuming 'cells' is your list of cell data
        html += f'<div class="grid-cell"><h3>{title}</h3>'
        for link_text, script in links:
            html += f'<a href="http://localhost:8501/?script={script}" target="_blank">{link_text}</a><br>'
        html += '</div>'
    html += '</div>'

    st.markdown(html, unsafe_allow_html=True)

    st.info("Tip: Right-click links and select 'Open in New Tab' to quickly open multiple editors.")
else:
    if script_to_run in editors:
        editors[script_to_run]()
    else:
        st.error(f"Unknown editor: {script_to_run}")