# ./CharacterManagement/JobEditor/interface.py

import streamlit as st
from .database import get_job_classes
from .forms import (
    render_filters,
    render_class_selection,
    render_job_class_form
)

def render_job_editor():
    """Render the job class editor interface"""
    st.header("Job Class Editor")

    # Initialize session state
    if 'selected_job_id' not in st.session_state:
        st.session_state.selected_job_id = None
        
    # Render filters
    search, type_filter, category_filter = render_filters()

    # Get filtered classes
    classes = get_job_classes(
        search=search,
        type_filter=type_filter,
        category_filter=category_filter
    )

    # Class selection
    selected_class = render_class_selection(classes)
    
    # Render edit form if class selected
    if selected_class:
        render_job_class_form(selected_class)