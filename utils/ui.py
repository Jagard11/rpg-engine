# ./utils/ui.py

import streamlit as st
from typing import List, Dict, Callable, Any

def render_dropdown(label: str, items: List[Dict], key: str, value_key: str = 'id', display_key: str = 'name', default_value: Any = None) -> Any:
    """Render a customizable dropdown from a list of dictionaries."""
    if not items:
        return None
    index = next((i for i, item in enumerate(items) if item[value_key] == default_value), 0)
    return st.selectbox(
        label,
        options=[item[value_key] for item in items],
        format_func=lambda x: next(item[display_key] for item in items if item[value_key] == x),
        index=index,
        key=key
    )