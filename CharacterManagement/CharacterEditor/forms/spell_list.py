# ./CharacterManagement/CharacterEditor/forms/spell_list.py

import streamlit as st

def render_spell_list(character_data: dict = None):
    """Render the Spell List subtab (placeholder)."""
    st.subheader("Spell List")
    st.info(
        """
        **Planned Functionality:**
        - Show all spells and abilities unlocked from racial/job classes and other sources.
        - Display prepared vs. unprepared spells (max 300 total, 120 prepared at level 100, adjustable in safe areas).
        - Include debuffs from classes (e.g., undead skeletons: weak to fire/holy, resistant to piercing).
        - Force selection of 3 spells per job class level-up from the class's spell list.
        - Allow toggling preparation status.
        - Current Status: Placeholder - awaiting spell system integration.
        """
    )
    if character_data:
        st.write(f"Character ID: {character_data.get('id', 'N/A')}")
        st.write("Spell and ability list not yet implemented.")