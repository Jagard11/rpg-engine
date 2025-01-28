# ./SpellManager/spellManager.py

from typing import Dict, List
import streamlit as st
from .spellList import render_spell_list
from .spellEditor import render_spell_editor

def render_spell_manager():
    """Main wrapper for the spell management interface"""
    st.header("Spell Manager")
    
    # Initialize session state for active tab if not exists
    if 'spell_manager_tab' not in st.session_state:
        st.session_state.spell_manager_tab = "Spell List"
    
    # Tab selection
    selected_tab = st.radio(
        "Choose View",
        ["Spell List", "Spell Editor"],
        key="spell_manager_tab_selector",
        horizontal=True,
        label_visibility="hidden"
    )
    
    # Update session state
    st.session_state.spell_manager_tab = selected_tab
    
    # Render appropriate component
    if selected_tab == "Spell List":
        render_spell_list()
    else:
        render_spell_editor()