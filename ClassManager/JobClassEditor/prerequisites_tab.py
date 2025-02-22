# ./ClassManager/JobClassEditor/prerequisites_tab.py

import streamlit as st

def render_prerequisites_tab():
    """Render the Prerequisites tab"""
    st.subheader("Prerequisites")
    for i, prereq in enumerate(st.session_state.class_prerequisites):
        st.write(f"Prerequisite {i+1}")
        col1, col2, col3, col4 = st.columns(4)
        with col1:
            prereq['prerequisite_group'] = st.number_input("Group", min_value=1, value=prereq.get('prerequisite_group', 1), key=f"prereq_group_{i}")
        with col2:
            prereq['prerequisite_type'] = st.text_input("Type", value=prereq.get('prerequisite_type', ''), key=f"prereq_type_{i}")
        with col3:
            prereq['target_id'] = st.number_input("Target ID", value=prereq.get('target_id', 0), key=f"prereq_target_id_{i}")
        with col4:
            prereq['required_level'] = st.number_input("Required Level", min_value=0, value=prereq.get('required_level', 0), key=f"prereq_level_{i}")
        col5, col6, col7 = st.columns(3)
        with col5:
            prereq['min_value'] = st.number_input("Min Value", value=prereq.get('min_value', 0), key=f"prereq_min_{i}")
        with col6:
            prereq['max_value'] = st.number_input("Max Value", value=prereq.get('max_value', 0), key=f"prereq_max_{i}")
        with col7:
            if st.checkbox("Remove", key=f"remove_prereq_{i}"):
                st.session_state.class_prerequisites.pop(i)
                st.rerun()

    with st.expander("Add New Prerequisite"):
        new_group = st.number_input("Group", min_value=1, value=1, key="new_prereq_group")
        new_type = st.text_input("Type", key="new_prereq_type")
        new_target_id = st.number_input("Target ID", value=0, key="new_prereq_target_id")
        new_level = st.number_input("Required Level", min_value=0, value=0, key="new_prereq_level")
        new_min = st.number_input("Min Value", value=0, key="new_prereq_min")
        new_max = st.number_input("Max Value", value=0, key="new_prereq_max")
        st.checkbox("Add this prerequisite when saving", key="add_prereq_checkbox")