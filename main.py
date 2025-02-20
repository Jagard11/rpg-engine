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
    "job": render_job_editor,
    "job_prerequisites": render_job_prerequisites,
    "job_conditions": render_job_conditions,
    "job_spell_list": render_job_spell_list,
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

    # Class Editor Section
    with st.expander("Class Editor", expanded=True):
        st.markdown("<a href='http://localhost:8501/?script=job' target='_blank'>Job Class Editor</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=job_prerequisites' target='_blank'>Job Prerequisites</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=job_conditions' target='_blank'>Job Conditions</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=job_spell_list' target='_blank'>Job Spell List</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=race' target='_blank'>Race Class Editor</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=race_equipment_slots' target='_blank'>Race Equipment Slots</a>", unsafe_allow_html=True)

    # Character Editor Section
    with st.expander("Character Editor", expanded=True):
        st.markdown("<a href='http://localhost:8501/?script=character' target='_blank'>Character Editor (Basic Info)</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_history' target='_blank'>Character History</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_equipment' target='_blank'>Equipment</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_level_distribution' target='_blank'>Level Distribution</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_spell_list' target='_blank'>Spell List</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_quests' target='_blank'>Completed Quests</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=character_achievements' target='_blank'>Earned Achievements</a>", unsafe_allow_html=True)

    # Spell Editor Section
    with st.expander("Spell Editor", expanded=True):
        st.markdown("<a href='http://localhost:8501/?script=spell_effect' target='_blank'>Spell Effect Editor</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=spell_wrappers' target='_blank'>Spell Wrappers</a>", unsafe_allow_html=True)

    # Additional Tools Section
    with st.expander("Additional Tools", expanded=True):
        st.markdown("<a href='http://localhost:8501/?script=location' target='_blank'>Location Editor</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=db_inspector' target='_blank'>Database Inspector</a>", unsafe_allow_html=True)
        st.markdown("<a href='http://localhost:8501/?script=server' target='_blank'>Server Message</a>", unsafe_allow_html=True)

    st.info("Tip: Right-click links and select 'Open in New Tab' to quickly open multiple editors.")
else:
    if script_to_run in editors:
        editors[script_to_run]()
    else:
        st.error(f"Unknown editor: {script_to_run}")