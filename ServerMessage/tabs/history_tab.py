# ./ServerMessage/tabs/history_tab.py

import streamlit as st

def render_history_tab():
    """Render the chat history tab"""
    st.header("Chat History")
    
    if st.button("Clear History", key="clear_history_btn"):
        st.session_state.chat_history = []
        st.experimental_rerun()
    
    for message in st.session_state.chat_history:
        with st.container():
            if message["is_user"]:
                st.markdown("**User:**")
            else:
                st.markdown("**Assistant:**")
            st.write(message["content"])
            st.divider()