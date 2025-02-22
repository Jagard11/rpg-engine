# ./CharacterManagement/JobEditor/forms/prerequisites.py

import streamlit as st

def render_job_prerequisites(job_data: dict = None):
    """Render the Prerequisites subtab (placeholder) for Job Class."""
    st.subheader("Prerequisites")
    st.info(
        """
        **Planned Functionality:**
        - Define prerequisites for this job class to appear on the level-up screen (e.g., specific class levels, karma).
        - Group prerequisites into AND/OR conditions (similar to Race Editor).
        - Save to class_prerequisites table with job-specific logic.
        - Current Status: Placeholder - to mirror Race Editor’s prerequisite system once fully designed.
        """
    )
    if job_data:
        st.write(f"Job ID: {job_data.get('id', 'N/A')}")
        st.write("Prerequisite conditions not yet implemented.")