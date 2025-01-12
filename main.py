import streamlit as st
from ServerMessage import render_server_tab, init_server_state
from CharacterInfo import init_character_state, render_character_tab

# Initialize all states
init_server_state()
init_character_state()

# Main app with tabs
st.set_page_config(page_title="RPG Communication Interface", layout="wide")

tab1, tab2 = st.tabs(["Server Messages", "Character"])

with tab1:
    render_server_tab()
    
with tab2:
    render_character_tab()