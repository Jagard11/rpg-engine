# ./CharacterManager/quests.py

import streamlit as st

def render_completed_quests(character_data: dict = None):
    """Render the Completed Quests subtab (placeholder)."""
    st.subheader("Completed Quests")
    st.info(
        """
        **Planned Functionality:**
        - List all currently active quests.
        - List all quests the character has completed.
        - Show quest details (e.g., name, description, rewards, completion date).
        - Link to a quest system for tracking and management.
        - Integrate with achievements and history systems.
        - Current Status: Placeholder - awaiting quest system design.
        """
    )
    if character_data:
        st.write(f"Character ID: {character_data.get('id', 'N/A')}")
        st.write("Completed quests not yet implemented.")