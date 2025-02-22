# ./ClassManager/JobClassEditor/exclusions_tab.py

import streamlit as st

def render_exclusions_tab():
    """Render the Exclusions tab"""
    st.subheader("Exclusions")
    for i, excl in enumerate(st.session_state.class_exclusions):
        st.write(f"Exclusion {i+1}")
        col1, col2, col3, col4 = st.columns(4)
        with col1:
            excl['exclusion_type'] = st.text_input("Type", value=excl.get('exclusion_type', ''), key=f"excl_type_{i}")
        with col2:
            excl['target_id'] = st.number_input("Target ID", value=excl.get('target_id', 0), key=f"excl_target_id_{i}")
        with col3:
            excl['min_value'] = st.number_input("Min Value", value=excl.get('min_value', 0), key=f"excl_min_{i}")
        with col4:
            excl['max_value'] = st.number_input("Max Value", value=excl.get('max_value', 0), key=f"excl_max_{i}")
        col5 = st.columns(1)[0]
        with col5:
            if st.checkbox("Remove", key=f"remove_excl_{i}"):
                st.session_state.class_exclusions.pop(i)
                st.rerun()

    with st.expander("Add New Exclusion"):
        new_excl_type = st.text_input("Type", key="new_excl_type")
        new_excl_target_id = st.number_input("Target ID", value=0, key="new_excl_target_id")
        new_excl_min = st.number_input("Min Value", value=0, key="new_excl_min")
        new_excl_max = st.number_input("Max Value", value=0, key="new_excl_max")
        st.checkbox("Add this exclusion when saving", key="add_excl_checkbox")