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

    # Get mode and edit_id from query parameters
    mode = st.query_params.get("mode", None)
    edit_id = st.query_params.get("edit_id", None)

    # Initialize session state
    if 'selected_job_id' not in st.session_state:
        st.session_state.selected_job_id = None

    # If mode is "create", show the form for a new job class
    if mode == "create":
        render_job_class_form(None, mode="create")
        return

    # If edit_id is provided, show the form for editing an existing job class
    elif edit_id:
        try:
            job_id = int(edit_id)
            selected_class = get_job_class_by_id(job_id)
            if selected_class:
                st.session_state.selected_job_id = job_id
                render_job_class_form(selected_class, mode="edit", record_id=job_id)
                return
            else:
                st.error(f"No job class found with ID {job_id}")
                st.session_state.selected_job_id = None
        except ValueError:
            st.error("Invalid edit_id provided")
            st.session_state.selected_job_id = None

    # Otherwise, show the filter and selection UI
    search, type_filter, category_filter = render_filters()
    classes = get_job_classes(search=search, type_filter=type_filter, category_filter=category_filter)
    selected_class = render_class_selection(classes)
    if selected_class:
        render_job_class_form(selected_class, mode="edit", record_id=selected_class['id'])
    else:
        # Option to create a new job class
        if st.button("Create New Job Class"):
            render_job_class_form(None, mode="create")

# Call the function (though typically called from main.py)
if __name__ == "__main__":
    render_job_editor()