# ./main.py
import streamlit as st
from ServerMessage import render_server_tab  # Changed from server_communication
from CharacterInfo import init_character_state, render_character_tab  # Changed to match your file name

# Initialize character state
init_character_state()

# Main app with tabs
st.set_page_config(page_title="RPG Communication Interface", layout="wide")

tab1, tab2 = st.tabs(["Server Messages", "Character"])

with tab1:
    render_server_tab()
    
with tab2:
    render_character_tab()