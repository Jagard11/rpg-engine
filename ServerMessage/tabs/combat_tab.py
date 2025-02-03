# ./ServerMessage/tabs/combat_tab.py

import streamlit as st

def render_combat_tab():
    """Render the combat interface tab"""
    st.header("Combat Interface")

    # Initialize combat state if not exists
    if 'combat_active' not in st.session_state:
        st.session_state.combat_active = False
    if 'combat_round' not in st.session_state:
        st.session_state.combat_round = 1
    if 'combat_distance' not in st.session_state:
        st.session_state.combat_distance = 30
    if 'current_effects' not in st.session_state:
        st.session_state.current_effects = []

    # Combat Controls
    col1, col2, col3 = st.columns(3)
    with col1:
        if not st.session_state.combat_active:
            if st.button("Start Combat", key="start_combat_btn"):
                st.session_state.combat_active = True
                st.experimental_rerun()
        else:
            if st.button("End Combat", key="end_combat_btn"):
                st.session_state.combat_active = False
                st.session_state.combat_round = 1
                st.session_state.current_effects = []
                st.experimental_rerun()
    
    with col2:
        st.metric("Current Round", st.session_state.combat_round)
    
    with col3:
        st.metric("Distance", f"{st.session_state.combat_distance} ft")

    if st.session_state.combat_active:
        # Combatant Status
        col1, col2 = st.columns(2)
        
        with col1:
            st.subheader("Player Character")
            st.number_input("PC HP", min_value=0, value=100, key="pc_combat_hp")
            st.number_input("PC MP", min_value=0, value=100, key="pc_combat_mp")
            st.number_input("PC Special", min_value=0, value=100, key="pc_combat_special")
            
            # PC Actions
            with st.expander("Available Actions"):
                action_type = st.radio(
                    "Action Type",
                    ["Attack", "Spell", "Skill", "Move", "Item"],
                    key="pc_action_type"
                )
                
                if action_type == "Attack":
                    st.selectbox("Choose Attack", ["Basic Attack", "Power Attack"], key="pc_attack_select")
                elif action_type == "Spell":
                    st.selectbox("Choose Spell", ["Fireball", "Heal", "Shield"], key="pc_spell_select")
                    st.number_input("MP Cost", min_value=0, value=10, key="pc_spell_cost")
                elif action_type == "Move":
                    st.number_input("Movement Distance", min_value=0, max_value=30, value=5, key="pc_move_distance")
                elif action_type == "Item":
                    st.selectbox("Choose Item", ["Health Potion", "Mana Potion"], key="pc_item_select")

                if st.button("Execute Action", key="pc_execute_btn"):
                    pass  # Implement action execution

        with col2:
            st.subheader("NPC")
            st.number_input("NPC HP", min_value=0, value=100, key="npc_combat_hp")
            st.number_input("NPC MP", min_value=0, value=100, key="npc_combat_mp")
            st.number_input("NPC Special", min_value=0, value=100, key="npc_combat_special")
            
            # NPC Actions
            with st.expander("Available Actions"):
                action_type = st.radio(
                    "Action Type",
                    ["Attack", "Spell", "Skill", "Move", "Item"],
                    key="npc_action_type"
                )
                
                if action_type == "Attack":
                    st.selectbox("Choose Attack", ["Basic Attack", "Power Attack"], key="npc_attack_select")
                elif action_type == "Spell":
                    st.selectbox("Choose Spell", ["Fireball", "Heal", "Shield"], key="npc_spell_select")
                    st.number_input("MP Cost", min_value=0, value=10, key="npc_spell_cost")
                elif action_type == "Move":
                    st.number_input("Movement Distance", min_value=0, max_value=30, value=5, key="npc_move_distance")
                elif action_type == "Item":
                    st.selectbox("Choose Item", ["Health Potion", "Mana Potion"], key="npc_item_select")

                if st.button("Execute Action", key="npc_execute_btn"):
                    pass  # Implement action execution

        # Active Effects
        st.subheader("Active Effects")
        if st.session_state.current_effects:
            for effect in st.session_state.current_effects:
                st.write(f"{effect['name']} - {effect['duration']} rounds remaining")
        else:
            st.write("No active effects")

        # Combat Log
        with st.expander("Combat Log", expanded=True):
            if 'combat_log' not in st.session_state:
                st.session_state.combat_log = []
            
            for log_entry in st.session_state.combat_log:
                st.text(log_entry)

        # Next Round Button
        if st.button("Next Round", key="next_round_btn"):
            st.session_state.combat_round += 1
            # Update effects duration
            updated_effects = []
            for effect in st.session_state.current_effects:
                if effect['duration'] > 1:
                    effect['duration'] -= 1
                    updated_effects.append(effect)
            st.session_state.current_effects = updated_effects
            st.experimental_rerun()

def add_effect(name, duration):
    """Add a new effect to the combat"""
    if 'current_effects' not in st.session_state:
        st.session_state.current_effects = []
    
    st.session_state.current_effects.append({
        'name': name,
        'duration': duration
    })