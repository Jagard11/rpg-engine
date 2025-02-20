# ./SpellEffectManager/interface.py

import streamlit as st
from utils.state import state
from .database import get_spell_effects, get_spell_effect_details, save_spell_effect
from .forms import render_spell_effect_form

def render_spell_effect_editor():
    """Render the spell effect editor interface."""
    st.header("Spell Effect Editor")

    col1, col2 = st.columns([1, 3])
    with col1:
        st.subheader("Spell Effects")
        effects = get_spell_effects()
        effect_options = {f"{e['name']} ({e['effect_type_name']}, {e['magic_school_name']})": e['id'] for e in effects}
        effect_options["Create New"] = None
        selected_effect = st.selectbox("Select Spell Effect", options=list(effect_options.keys()))
        selected_id = effect_options[selected_effect]
        if selected_id != state.get('selected_effect_id'):
            state.set('selected_effect_id', selected_id)

    with col2:
        effect_data = get_spell_effect_details(state.get('selected_effect_id')) if state.get('selected_effect_id') else {}
        form_data = render_spell_effect_form(effect_data)
        if form_data:
            success, message = save_spell_effect(form_data)
            if success:
                st.success(message)
                state.set('selected_effect_id', None)
                st.rerun()
            else:
                st.error(message)