# ./SpellManager/spellManager.py

from typing import Dict, List
import streamlit as st
from .database import load_spells
from .spellList import render_spell_list
from .spellEditor import render_spell_editor

def render_spell_manager():
    """Main wrapper for the spell management interface"""
    st.header("Spell Manager")
    
    # Load spells for dropdown
    spells = load_spells()
    spell_options = {str(spell['id']): f"[{spell['id']}] {spell['name']}" for spell in spells}
    
    # Create dropdown for spell selection
    selected_spell_id = st.selectbox(
        "Select Spell Record",
        options=[""] + list(spell_options.keys()),
        format_func=lambda x: "Select a spell..." if x == "" else spell_options[x]
    )
    
    if selected_spell_id:
        spell_data = next((spell for spell in spells if str(spell['id']) == selected_spell_id), None)
        if spell_data:
            st.session_state.editing_spell = spell_data
    
    # Initialize session state for active tab if not exists
    if 'spell_manager_tab' not in st.session_state:
        st.session_state.spell_manager_tab = "Spell List"
    
    # Create subtabs
    list_tab, editor_tab = st.tabs(["Spell List", "Spell Editor"])
    
    with list_tab:
        render_spell_list()
    
    with editor_tab:
        render_spell_editor()