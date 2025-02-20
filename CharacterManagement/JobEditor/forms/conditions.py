# ./CharacterManagement/JobEditor/forms/conditions.py

import streamlit as st

def render_job_conditions(job_data: dict = None):
    """Render the Conditions subtab (placeholder) for Job Class."""
    st.subheader("Conditions Preventing Unlock")
    st.info(
        """
        **Planned Functionality:**
        - Define conditions that prevent characters from unlocking this job class (e.g., incompatible race, negative karma).
        - Use a similar AND/OR structure to prerequisites.
        - Save to a conditions table or integrate with prerequisites logic.
        - Current Status: Placeholder - awaiting condition system design.
        """
    )
    if job_data:
        st.write(f"Job ID: {job_data.get('id', 'N/A')}")
        st.write("Unlock prevention conditions not yet implemented.")