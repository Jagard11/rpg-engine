# ./CharacterManager/level_distribution.py

import streamlit as st

def render_level_distribution(character_data: dict = None):
    """Render the Level Distribution subtab (placeholder)."""
    st.subheader("Level Distribution")
    st.info(
        """
        **Planned Functionality:**
        - Display the character's racial level and job levels separately.
        - Show total level as the sum of racial and job levels (up to 100).
        - Allow viewing/editing level allocations within system limits (e.g., base classes: 15 levels, high: 10, rare: 5).
        - Integrate with Class Editor for level caps and prerequisites.
        - Current Status: Placeholder - awaiting level system integration.
        """
    )
    if character_data:
        st.write(f"Character ID: {character_data.get('id', 'N/A')}")
        st.write(f"Total Level: {character_data.get('total_level', 'N/A')}")
        st.write("Level distribution details not yet implemented.")