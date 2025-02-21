# ./CharacterManagement/JobEditor/interface.py

import streamlit as st
from .database import get_job_classes
from .job_forms import render_filters, render_class_selection, render_job_class_form

def get_job_class_by_id(job_id: int):
    """Fetch a specific job class by ID"""
    classes = get_job_classes()  # Fetch all job classes
    for cls in classes:
        if cls['id'] == job_id:
            return cls
    return None

def render_job_editor():
    """Render the job class editor interface"""
    st.header("Job Class Editor")

    # Check for edit_id from query parameters
    edit_id = st.query_params.get("edit_id", None)

    # Initialize session state
    if 'selected_job_id' not in st.session_state:
        st.session_state.selected_job_id = None

    # If edit_id is provided, load and display that job class directly
    if edit_id:
        try:
            job_id = int(edit_id)
            selected_class = get_job_class_by_id(job_id)
            if selected_class:
                st.session_state.selected_job_id = job_id
                render_job_class_form(selected_class, mode="edit", record_id=job_id)
                return  # Exit after rendering the form
            else:
                st.error(f"No job class found with ID {job_id}")
                st.session_state.selected_job_id = None
        except ValueError:
            st.error("Invalid edit_id provided")
            st.session_state.selected_job_id = None

    # Otherwise, check if a job is selected via the UI
    elif st.session_state.selected_job_id:
        selected_class = get_job_class_by_id(st.session_state.selected_job_id)
        if selected_class:
            render_job_class_form(selected_class, mode="edit", record_id=st.session_state.selected_job_id)
        else:
            st.error("Selected job class not found")
            st.session_state.selected_job_id = None

    # If no job is selected, show the filter and selection UI or create new
    else:
        search, type_filter, category_filter = render_filters()
        classes = get_job_classes(search=search, type_filter=type_filter, category_filter=category_filter)
        selected_class = render_class_selection(classes)
        if selected_class:
            render_job_class_form(selected_class, mode="edit", record_id=selected_class['id'])
        else:
            # Option to create a new job class
            if st.button("Create New Job Class"):
                render_job_class_form(None, mode="create")