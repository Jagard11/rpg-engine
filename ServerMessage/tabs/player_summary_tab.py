# ./ServerMessage/tabs/player_summary_tab.py

import streamlit as st

def render_player_summary_tab():
    """Render the Player character summary tab"""
    st.header("Player Character Summary")

    # Basic Info
    with st.expander("Basic Information", expanded=True):
        col1, col2 = st.columns(2)
        with col1:
            st.text_input("Character Name", key="pc_name_input")
            st.text_input("Race", key="pc_race_input")
            st.text_input("Primary Class", key="pc_primary_class_input")
        with col2:
            st.number_input("Level", min_value=1, max_value=100, value=1, key="pc_level_input")
            st.text_input("Secondary Class", key="pc_secondary_class_input")
            st.text_input("Alignment", key="pc_alignment_input")

    # Stats
    with st.expander("Statistics", expanded=True):
        col1, col2, col3 = st.columns(3)
        
        with col1:
            st.number_input("Strength", min_value=1, max_value=100, value=10, key="pc_str_input")
            st.number_input("Dexterity", min_value=1, max_value=100, value=10, key="pc_dex_input")
            st.number_input("Constitution", min_value=1, max_value=100, value=10, key="pc_con_input")
        
        with col2:
            st.number_input("Intelligence", min_value=1, max_value=100, value=10, key="pc_int_input")
            st.number_input("Wisdom", min_value=1, max_value=100, value=10, key="pc_wis_input")
            st.number_input("Charisma", min_value=1, max_value=100, value=10, key="pc_cha_input")
        
        with col3:
            st.number_input("HP", min_value=1, value=10, key="pc_hp_input")
            st.number_input("MP", min_value=0, value=10, key="pc_mp_input")
            st.number_input("Special", min_value=0, value=0, key="pc_special_input")

    # Skills and Spells
    with st.expander("Skills & Spells", expanded=True):
        tab1, tab2 = st.tabs(["Skills", "Spells"])
        
        with tab1:
            st.text_area("Skills", height=150, key="pc_skills_input")
            if st.button("Add Skill", key="pc_add_skill_btn"):
                pass  # Functionality to be added
        
        with tab2:
            st.text_area("Spells", height=150, key="pc_spells_input")
            if st.button("Add Spell", key="pc_add_spell_btn"):
                pass  # Functionality to be added

    # Equipment and Inventory
    with st.expander("Equipment & Inventory", expanded=True):
        tab1, tab2 = st.tabs(["Equipment", "Inventory"])
        
        with tab1:
            st.text_area("Equipment", height=150, key="pc_equipment_input")
            if st.button("Add Equipment", key="pc_add_equipment_btn"):
                pass  # Functionality to be added
        
        with tab2:
            st.text_area("Inventory", height=150, key="pc_inventory_input")
            if st.button("Add Item", key="pc_add_inventory_btn"):
                pass  # Functionality to be added

    # Character Background
    with st.expander("Character Background", expanded=True):
        st.text_area("Background Story", height=200, key="pc_background_input")
        st.text_input("Notable Achievements", key="pc_achievements_input")

    # Save/Load
    col1, col2 = st.columns(2)
    with col1:
        if st.button("Save Character", key="save_pc_btn"):
            pass  # Functionality to be added
    with col2:
        if st.button("Load Character", key="load_pc_btn"):
            pass  # Functionality to be added