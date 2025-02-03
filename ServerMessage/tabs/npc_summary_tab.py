# ./ServerMessage/tabs/npc_summary_tab.py

import streamlit as st

def render_npc_summary_tab():
    """Render the NPC character summary tab"""
    st.header("NPC Summary")

    # Basic Info
    with st.expander("Basic Information", expanded=True):
        st.text_input("Name", key="npc_name_input")
        st.text_input("Alignment", key="npc_alignment_input")

    # Level Progression
    with st.expander("Level Progression", expanded=True):
        st.subheader("Race Levels")
        
        # Dynamic race level entries
        if 'npc_race_levels' not in st.session_state:
            st.session_state.npc_race_levels = [{"race": "", "level": 1}]
        
        for i, race_level in enumerate(st.session_state.npc_race_levels):
            col1, col2, col3 = st.columns([3, 1, 1])
            with col1:
                race = st.selectbox(
                    "Race",
                    options=["Humanoid", "Demi-Human", "Heteromorphic"], 
                    key=f"npc_race_select_{i}"
                )
            with col2:
                level = st.number_input(
                    "Level",
                    min_value=1,
                    max_value=100,
                    value=race_level["level"],
                    key=f"npc_race_level_{i}"
                )
            with col3:
                if st.button("Remove", key=f"npc_remove_race_{i}"):
                    st.session_state.npc_race_levels.pop(i)
                    st.experimental_rerun()
        
        if st.button("Add Race Level", key="npc_add_race_btn"):
            st.session_state.npc_race_levels.append({"race": "", "level": 1})
            st.experimental_rerun()

        st.subheader("Class Levels")
        
        # Dynamic class level entries
        if 'npc_class_levels' not in st.session_state:
            st.session_state.npc_class_levels = [{"class": "", "level": 1}]
        
        for i, class_level in enumerate(st.session_state.npc_class_levels):
            col1, col2, col3, col4 = st.columns([3, 1, 2, 1])
            with col1:
                class_name = st.selectbox(
                    "Class",
                    options=["Magic Caster", "Martial", "Shadow", "Wayfarer", "Leader", "Laborer"],
                    key=f"npc_class_select_{i}"
                )
            with col2:
                level = st.number_input(
                    "Level",
                    min_value=1,
                    max_value=15,  # Base classes max at 15
                    value=class_level["level"],
                    key=f"npc_class_level_{i}"
                )
            with col3:
                st.text_input(
                    "Unlocked Abilities",
                    key=f"npc_class_abilities_{i}",
                    help="Abilities unlocked at this level"
                )
            with col4:
                if st.button("Remove", key=f"npc_remove_class_{i}"):
                    st.session_state.npc_class_levels.pop(i)
                    st.experimental_rerun()
        
        if st.button("Add Class Level", key="npc_add_class_btn"):
            st.session_state.npc_class_levels.append({"class": "", "level": 1})
            st.experimental_rerun()

        # Total level display
        total_levels = sum(rl["level"] for rl in st.session_state.npc_race_levels) + \
                      sum(cl["level"] for cl in st.session_state.npc_class_levels)
        st.info(f"Total Character Level: {total_levels}/100")

    # Stats
    with st.expander("Statistics", expanded=True):
        col1, col2, col3 = st.columns(3)
        
        with col1:
            st.number_input("Strength", min_value=1, max_value=100, value=10, key="npc_str_input")
            st.number_input("Dexterity", min_value=1, max_value=100, value=10, key="npc_dex_input")
            st.number_input("Constitution", min_value=1, max_value=100, value=10, key="npc_con_input")
        
        with col2:
            st.number_input("Intelligence", min_value=1, max_value=100, value=10, key="npc_int_input")
            st.number_input("Wisdom", min_value=1, max_value=100, value=10, key="npc_wis_input")
            st.number_input("Charisma", min_value=1, max_value=100, value=10, key="npc_cha_input")
        
        with col3:
            st.number_input("HP", min_value=1, value=10, key="npc_hp_input")
            st.number_input("MP", min_value=0, value=10, key="npc_mp_input")
            st.number_input("Special", min_value=0, value=0, key="npc_special_input")

    # Skills and Spells
    with st.expander("Skills & Spells", expanded=True):
        tab1, tab2 = st.tabs(["Skills", "Spells"])
        
        with tab1:
            st.text_area("Skills", height=150, key="npc_skills_input")
            if st.button("Add Skill", key="npc_add_skill_btn"):
                pass  # Functionality to be added
        
        with tab2:
            st.text_area("Spells", height=150, key="npc_spells_input")
            if st.button("Add Spell", key="npc_add_spell_btn"):
                pass  # Functionality to be added

    # Equipment
    with st.expander("Equipment", expanded=True):
        st.text_area("Equipment", height=150, key="npc_equipment_input")
        if st.button("Add Equipment", key="npc_add_equipment_btn"):
            pass  # Functionality to be added

    # Save/Load
    col1, col2 = st.columns(2)
    with col1:
        if st.button("Save NPC", key="save_npc_btn"):
            pass  # Functionality to be added
    with col2:
        if st.button("Load NPC", key="load_npc_btn"):
            pass  # Functionality to be added