# ./CharacterManagement/JobEditor/forms/spell_list.py

import streamlit as st

def render_job_spell_list(job_data: dict = None):
    """Render the Spell List subtab (placeholder) for Job Class."""
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
    if job_data:
        st.write(f"Job ID: {job_data.get('id', 'N/A')}")
        st.write("Spell list assignment not yet implemented.")