# ./ClassManager/__init__.py

import streamlit as st
from typing import Literal
import pandas as pd
from .JobClassEditor.classesTable import render_job_table
from .JobClassEditor.class_editor import render_class_editor

def render_class_manager(view: Literal["list", "editor", "spells"]):
    """Main rendering function for the class management system
    
    Args:
        view (str): Which view to render - "list", "editor", or "spells"
    """
    # Initialize session state if needed
    if 'class_query_results' not in st.session_state:
        st.session_state.class_query_results = pd.DataFrame()
        
    match view:
        case "list":
            render_job_table()
        case "editor":
            render_class_editor()
        case _:
            st.error(f"Unknown view type: {view}")