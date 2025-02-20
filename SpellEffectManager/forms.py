# ./SpellEffectManager/forms.py

import streamlit as st
from utils.ui import render_dropdown
from .database import get_reference_data
from typing import Dict

def render_spell_effect_form(effect_data: Dict) -> Dict:
    """Render the spell effect form."""
    with st.form("spell_effect_form"):
        col1, col2 = st.columns(2)
        with col1:
            name = st.text_input("Name", value=effect_data.get('name', ''))
        with col2:
            effect_type_id = render_dropdown(
                "Effect Type", get_reference_data('effect_types'), "effect_type",
                default_value=effect_data.get('effect_type_id')
            )
        description = st.text_area("Description", value=effect_data.get('description', ''))

        col1, col2 = st.columns(2)
        with col1:
            magic_school_id = render_dropdown(
                "Magic School", get_reference_data('magic_schools'), "magic_school",
                default_value=effect_data.get('magic_school_id')
            )

        effect_type_name = next((et['name'] for et in get_reference_data('effect_types') if et['id'] == effect_type_id), '')
        damage_data = effect_data.get('damage_data', {})
        if effect_type_name == 'damage':
            st.subheader("Damage Effect Details")
            col1, col2 = st.columns(2)
            with col1:
                range_type_id = render_dropdown(
                    "Range Type", get_reference_data('range_types'), "range_type",
                    default_value=damage_data.get('range_type_id')
                )
                range_distance = st.number_input("Range Distance", value=damage_data.get('range_distance', 0))
            with col2:
                base_damage = st.text_input("Base Damage Formula", value=damage_data.get('base_damage', ''))
                resistance_save_id = render_dropdown(
                    "Resistance Save Stat", get_reference_data('stat_types'), "resistance_save",
                    default_value=damage_data.get('resistance_save_id')
                )

        if st.form_submit_button("Save"):
            if not name:
                st.error("Name is required!")
                return {}
            data = {
                'id': effect_data.get('id'),
                'name': name,
                'description': description,
                'effect_type_id': effect_type_id,
                'magic_school_id': magic_school_id,
                'effect_type_name': effect_type_name
            }
            if effect_type_name == 'damage':
                data['damage_data'] = {
                    'range_type_id': range_type_id,
                    'range_distance': range_distance,
                    'base_damage': base_damage,
                    'resistance_save_id': resistance_save_id
                }
            return data
    return {}
