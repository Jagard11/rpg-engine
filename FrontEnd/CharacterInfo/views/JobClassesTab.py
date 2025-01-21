# ./FrontEnd/CharacterInfo/views/JobClassesTab.py

import streamlit as st
from .JobClasses import render_job_classes_interface

def render_job_classes_tab():
    """Render the Job Classes management tab"""
    tab1, tab2 = st.tabs(["Browse Classes", "Edit/Create Class"])

    with tab1:
        # Initialize session state for class browsing
        if 'browse_category_filter' not in st.session_state:
            st.session_state.browse_category_filter = "All"
        if 'browse_type_filter' not in st.session_state:
            st.session_state.browse_type_filter = "All"
        if 'show_prerequisites' not in st.session_state:
            st.session_state.show_prerequisites = False
            
        st.header("Class Browser")
        render_job_classes_interface(browse_mode=True)

    with tab2:
        # Initialize session state for class editing
        if 'edit_category_filter' not in st.session_state:
            st.session_state.edit_category_filter = "All"
        if 'edit_type_filter' not in st.session_state:
            st.session_state.edit_type_filter = "All"
            
        st.header("Class Editor")
        render_job_classes_interface(browse_mode=False)