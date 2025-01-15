# ./main.py

import streamlit as st
from ServerMessage import render_server_tab
from CharacterInfo import init_character_state, render_character_tab
from DatabaseInspector import render_db_inspector_tab

# Initialize character state
init_character_state()

# Main app with tabs
st.set_page_config(page_title="RPG Communication Interface", layout="wide")

tab1, tab2, tab3 = st.tabs(["Server Messages", "Character", "Database Inspector"])

with tab1:
    render_server_tab()
    
with tab2:
    render_character_tab()

with tab3:
    render_db_inspector_tab()