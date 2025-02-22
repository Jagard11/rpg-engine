# ./ClassManager/JobClassEditor/spell_list_tab.py

import streamlit as st
from typing import Dict, Any

def render_spell_list_tab(current_record: Dict[str, Any]):
    """Render the Spell List tab"""
    st.subheader("Spell List")
    st.info(
        """
        **Planned Functionality:**
        - Assign spells and abilities to this job class’s spell list.
        - Define which spells are available at each level (e.g., 3 per level-up selection).
        - Link to Spell Editor for spell effect and wrapper definitions.
        - Current Status: Placeholder - awaiting spell system integration.
        """
    )
    if current_record:
        st.write(f"Class ID: {current_record.get('id', 'N/A')}")
        st.write("Spell list assignment not yet implemented.")
    if st.session_state.current_class_id != 0:
        spell_action = st.selectbox(
            "Spell List Actions",
            ["Select an action", "Edit Spell List"],
            key="spell_list_action"
        )
        if spell_action == "Edit Spell List":
            st.session_state.view = "spells"
            st.session_state.spell_list_action = "Select an action"
            st.rerun()