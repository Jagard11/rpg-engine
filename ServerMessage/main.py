# ./ServerMessage/main.py

import streamlit as st
import os
from tabs.chat_tab import render_chat_tab
from tabs.history_tab import render_history_tab
from tabs.git_tab import render_git_tab
from tabs.npc_summary_tab import render_npc_summary_tab
from tabs.player_summary_tab import render_player_summary_tab
from tabs.combat_tab import render_combat_tab
from tabs.debug_tab import render_debug_tab

# Initialize session states
if 'chat_history' not in st.session_state:
    st.session_state.chat_history = []
if 'git_output' not in st.session_state:
    st.session_state.git_output = None
if 'show_git_results' not in st.session_state:
    st.session_state.show_git_results = False

# Determine the directory of this script (./ServerMessage)
script_dir = os.path.dirname(os.path.abspath(__file__))
# Assume the repository root is one level up from the script directory.
repo_root = os.path.abspath(os.path.join(script_dir, ".."))

st.title("Oobabooga API Client (ServerMessage)")

# --- Configure API endpoint ---
with st.sidebar:
    st.header("Server Configuration")
    if 'server_ip' not in st.session_state:
        st.session_state.server_ip = "127.0.0.1"
    if 'server_port' not in st.session_state:
        st.session_state.server_port = "5000"
        
    st.session_state.server_ip = st.text_input("Oobabooga Server IP", 
                                              value=st.session_state.server_ip, 
                                              key="server_ip_input")
    st.session_state.server_port = st.text_input("Oobabooga Server Port", 
                                                value=st.session_state.server_port, 
                                                key="server_port_input")
    base_url = f"http://{st.session_state.server_ip}:{st.session_state.server_port}/v1"
    st.write("Using API Base URL:", base_url)

# Create tabs for different functionalities
chat_tab, params_tab, history_tab, char_tab, combat_tab, debug_tab, git_tab = st.tabs([
    "Chat Interface",
    "Parameters", 
    "Chat History", 
    "Character Summary",
    "Combat",
    "Debug",
    "Git Updates"
])

# Render each tab
with chat_tab:
    render_chat_tab(base_url)

with params_tab:
    st.header("Model Parameters")
    temperature = st.slider(
        "Temperature",
        min_value=0.1,
        max_value=2.0,
        value=0.7,
        step=0.1,
        key="temp_slider_params"
    )
    max_tokens = st.number_input(
        "Max Tokens",
        min_value=1,
        max_value=2048,
        value=200,
        key="max_tokens_input_params"
    )
    context_length = st.number_input(
        "Context Length",
        min_value=1,
        max_value=8192,
        value=2048,
        key="context_length_input_params"
    )
    st.session_state.temperature = temperature
    st.session_state.max_tokens = max_tokens
    st.session_state.context_length = context_length

with history_tab:
    render_history_tab()

with char_tab:
    # Create subtabs for NPC and Player character summaries
    npc_tab, player_tab = st.tabs(["NPC", "Player Character"])
    
    with npc_tab:
        render_npc_summary_tab()
    
    with player_tab:
        render_player_summary_tab()

with combat_tab:
    render_combat_tab()

with debug_tab:
    render_debug_tab()

with git_tab:
    render_git_tab(script_dir, repo_root)