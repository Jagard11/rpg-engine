# ./utils/state.py

import streamlit as st

class StateManager:
    """Centralized session state management."""
    @staticmethod
    def get(key: str, default=None):
        """Get a session state value, initializing with default if not set."""
        if key not in st.session_state:
            st.session_state[key] = default
        return st.session_state[key]

    @staticmethod
    def set(key: str, value):
        """Set a session state value."""
        st.session_state[key] = value

state = StateManager()