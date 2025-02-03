# ./ServerMessage/tabs/debug_tab.py

import streamlit as st

def render_debug_tab():
    """Render the debug information tab"""
    st.header("Debug Information")
    
    if hasattr(st.session_state, 'last_payload'):
        st.write("Last sent payload:")
        st.json(st.session_state.last_payload)
    
    if hasattr(st.session_state, 'last_response'):
        st.write("Last server response:")
        st.json(st.session_state.last_response)
        
    with st.expander("Session State", expanded=False):
        st.write(st.session_state)