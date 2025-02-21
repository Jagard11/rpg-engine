# ./ClassManager/__init__.py

import streamlit as st
from typing import Literal
import pandas as pd
from .classesTable import render_job_table
from .classEditor import render_class_editor
from .classSpellList import render_class_spell_list

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
        case "spells":
            render_class_spell_list()
        case _:
            st.error(f"Unknown view type: {view}")