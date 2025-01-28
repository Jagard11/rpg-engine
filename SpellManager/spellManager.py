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
        # Add SQL query input field
        default_query = """
            SELECT id, name, tier_name, mp_cost, damage_base, healing_base 
            FROM spells 
            JOIN spell_tiers ON spells.spell_tier = spell_tiers.id
            ORDER BY spell_tier, name
        """
        
        with st.expander("SQL Query", expanded=True):
            st.markdown("#### Custom SQL Query")
            st.markdown("Enter a custom SQL query to filter spells. Query must include `id` and `name` fields.")
            query = st.text_area(
                "Query",
                value=default_query,
                height=150,
                key="spell_query"
            )
            
            # Add execute button
            col1, col2 = st.columns([1, 5])
            with col1:
                st.session_state.execute_query = st.button("Execute Query")
        
        # Add horizontal line for visual separation
        st.markdown("---")
        
        # Render the spell list with the query
        render_spell_list()
    
    with editor_tab:
        render_spell_editor()