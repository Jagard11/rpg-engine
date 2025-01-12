# ./CharacterInfo.py

from dataclasses import dataclass
from typing import Dict, Optional
import streamlit as st

@dataclass
class Character:
    name: str
    level: int
    health: int
    mana: int
    strength: int
    dexterity: int
    intelligence: int
    constitution: int
    abilities: Dict[str, str]
    description: Optional[str] = None

def init_character_state():
    """Initialize character-related session state variables"""
    if 'character' not in st.session_state:
        st.session_state.character = Character(
            name="",
            level=1,
            health=100,
            mana=100,
            strength=10,
            dexterity=10,
            intelligence=10,
            constitution=10,
            abilities={},
            description=""
        )

def render_character_tab():
    """Render the character information tab"""
    st.header("Character Information")
    
    # Basic Info
    col1, col2 = st.columns(2)
    with col1:
        name = st.text_input("Character Name", value=st.session_state.character.name)
        level = st.number_input("Level", min_value=1, value=st.session_state.character.level)
        
    with col2:
        health = st.number_input("Health", min_value=0, value=st.session_state.character.health)
        mana = st.number_input("Mana", min_value=0, value=st.session_state.character.mana)
    
    # Stats
    st.subheader("Stats")
    col1, col2, col3, col4 = st.columns(4)
    
    with col1:
        strength = st.number_input("Strength", min_value=0, value=st.session_state.character.strength)
    with col2:
        dexterity = st.number_input("Dexterity", min_value=0, value=st.session_state.character.dexterity)
    with col3:
        intelligence = st.number_input("Intelligence", min_value=0, value=st.session_state.character.intelligence)
    with col4:
        constitution = st.number_input("Constitution", min_value=0, value=st.session_state.character.constitution)
    
    # Abilities
    st.subheader("Abilities")
    ability_name = st.text_input("New Ability Name")
    ability_description = st.text_area("Ability Description")
    
    if st.button("Add Ability"):
        if ability_name and ability_description:
            st.session_state.character.abilities[ability_name] = ability_description
            
    # Display existing abilities
    for name, desc in st.session_state.character.abilities.items():
        with st.expander(f"Ability: {name}"):
            st.write(desc)
            if st.button(f"Remove {name}"):
                del st.session_state.character.abilities[name]
    
    # Character Description
    st.subheader("Character Description")
    description = st.text_area("Description", value=st.session_state.character.description or "")
    
    # Update character state
    st.session_state.character = Character(
        name=name,
        level=level,
        health=health,
        mana=mana,
        strength=strength,
        dexterity=dexterity,
        intelligence=intelligence,
        constitution=constitution,
        abilities=st.session_state.character.abilities.copy(),
        description=description
    )
    
    # Save/Load functionality
    if st.button("Save Character"):
        save_character()
        
    if st.button("Load Character"):
        load_character()

def save_character():
    """Save character data to a file"""
    import json
    from pathlib import Path
    
    save_dir = Path("SaveData")
    save_dir.mkdir(exist_ok=True)
    
    character_data = {
        "name": st.session_state.character.name,
        "level": st.session_state.character.level,
        "health": st.session_state.character.health,
        "mana": st.session_state.character.mana,
        "strength": st.session_state.character.strength,
        "dexterity": st.session_state.character.dexterity,
        "intelligence": st.session_state.character.intelligence,
        "constitution": st.session_state.character.constitution,
        "abilities": st.session_state.character.abilities,
        "description": st.session_state.character.description
    }
    
    save_path = save_dir / f"{st.session_state.character.name}.json"
    with open(save_path, 'w') as f:
        json.dump(character_data, f, indent=2)
    st.success(f"Character saved to {save_path}")

def load_character():
    """Load character data from a file"""
    import json
    from pathlib import Path
    
    save_dir = Path("SaveData")
    if not save_dir.exists():
        st.error("No saved characters found!")
        return
        
    save_files = list(save_dir.glob("*.json"))
    if not save_files:
        st.error("No saved characters found!")
        return
        
    selected_file = st.selectbox("Select character to load", save_files)
    if selected_file:
        with open(selected_file, 'r') as f:
            data = json.load(f)
            st.session_state.character = Character(**data)
            st.success("Character loaded successfully!")