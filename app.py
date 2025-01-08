import streamlit as st
import requests
from datetime import datetime

# Configure Streamlit page to use wide mode
st.set_page_config(layout="wide")

# Initialize session state variables if they don't exist
if 'messages' not in st.session_state:
    st.session_state.messages = []
    
if 'server_message' not in st.session_state:
    st.session_state.server_message = ""
    
if 'server_response' not in st.session_state:
    st.session_state.server_response = ""
    
if 'server_history' not in st.session_state:
    st.session_state.server_history = []
    
if 'char_name' not in st.session_state:
    st.session_state.char_name = "Saronia"
    
if 'player_name' not in st.session_state:
    st.session_state.player_name = "Hero"
    
# Initialize default content if not in session state
if 'loaded_npc_card' not in st.session_state:
    st.session_state.loaded_npc_card = f"""Name: {st.session_state.char_name}
Role: Battle Mage
Personality: Scholarly and curious
Background: Trained at the Academy of High Magic
Appearance: Robed figure with glowing staff
Notable Traits: Speaks in a formal manner, values knowledge"""

if 'loaded_player_card' not in st.session_state:
    st.session_state.loaded_player_card = f"""Name: {st.session_state.player_name}
Role: Warrior
Personality: Bold and determined
Background: Former guard captain
Appearance: Heavily armored with shield and sword
Notable Traits: Natural leader, protective of allies"""

if 'loaded_npc_attacks' not in st.session_state:
    st.session_state.loaded_npc_attacks = """Staff Strike: A quick melee attack with the staff
Arcane Thrust: Channel magic through the staff for a piercing attack
Defensive Stance: Increase defense for 2 rounds"""

if 'loaded_npc_spells' not in st.session_state:
    st.session_state.loaded_npc_spells = """Fireball: Deals area fire damage
Ice Shield: Protects against physical attacks
Healing Light: Removes one negative status effect"""

if 'loaded_player_attacks' not in st.session_state:
    st.session_state.loaded_player_attacks = """Sword Slash: Basic melee attack
Shield Bash: Chance to stun target
Defensive Position: Raise shield to block"""

if 'loaded_player_spells' not in st.session_state:
    st.session_state.loaded_player_spells = """Healing Prayer: Restore health
Lightning Strike: Single target lightning damage
Purify: Remove poison effect"""

def send_to_server(message):
    """Send message to the API and return the response"""
    URL = "http://127.0.0.1:5000/v1/chat/completions"
    
    request_data = {
        "messages": [{"role": "user", "content": message}],
        "mode": "instruct",
        "instruction_template": "Alpaca",
        "temperature": 0.7,
        "max_tokens": 500
    }
    
    try:
        response = requests.post(
            URL,
            headers={"Content-Type": "application/json"},
            json=request_data,
            verify=False
        )
        
        if response.status_code == 200:
            return response.json()['choices'][0]['message']['content']
        else:
            return f"Error: {response.status_code} - {response.text}"
    except Exception as e:
        return f"Error: {str(e)}"

def replace_variables(text):
    """Replace variable placeholders in text"""
    return text.replace("<char_name>", st.session_state.char_name).replace("<player_name>", st.session_state.player_name)

# Streamlit UI
st.title("RPG Server Communication Debugger")

# Top section - Names and basic info
name_col1, name_col2, name_col3 = st.columns(3)

with name_col1:
    new_char_name = st.text_input("Character Name", st.session_state.char_name)
    if new_char_name != st.session_state.char_name:
        st.session_state.char_name = new_char_name
        st.rerun()

with name_col2:
    new_player_name = st.text_input("Player Name", st.session_state.player_name)
    if new_player_name != st.session_state.player_name:
        st.session_state.player_name = new_player_name
        st.rerun()

# Main content area
data_col, server_col = st.columns([1.2, 0.8])

with data_col:
    st.header("Data Fields")
    
    # Create three columns for better organization
    field_col1, field_col2, field_col3 = st.columns(3)
    
    def save_character_data(char_name, card_content, attacks_content, spells_content):
        """Save character data to a text file"""
        content = f"""CHARACTER NAME: {char_name}

CHARACTER CARD:
{card_content}

ATTACKS:
{attacks_content}

SPELLS:
{spells_content}
"""
        
        try:
            filename = f"character_{char_name.lower().replace(' ', '_')}.txt"
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        except Exception as e:
            st.error(f"Error saving file: {str(e)}")
            return False

    def load_character_data(file_data, is_player=False):
        """Parse character data from file content"""
        content = file_data.decode('utf-8')
        sections = content.split('\n\n')
        
        data = {}
        current_section = None
        
        for section in sections:
            if section.startswith('CHARACTER NAME:'):
                data['name'] = section.replace('CHARACTER NAME:', '').strip()
            elif section.startswith('CHARACTER CARD:'):
                data['card'] = section.replace('CHARACTER CARD:', '').strip()
            elif section.startswith('ATTACKS:'):
                data['attacks'] = section.replace('ATTACKS:', '').strip()
            elif section.startswith('SPELLS:'):
                data['spells'] = section.replace('SPELLS:', '').strip()
        
        return data

    with field_col1:
        # Character Card
        with st.expander(f"{st.session_state.char_name}'s Character Card", expanded=True):
            character_card = st.text_area(
                "Character Description",
                value=f"""Name: {st.session_state.char_name}
Role: Battle Mage
Personality: Scholarly and curious
Background: Trained at the Academy of High Magic
Appearance: Robed figure with glowing staff
Notable Traits: Speaks in a formal manner, values knowledge""",
                height=150,
                key="npc_card"
            )
            
            # NPC Character Buttons
            if st.button("Add to Message", key="add_npc_card"):
                st.session_state.server_message += f"\nCharacter Description:\n{character_card}"
                st.rerun()
                
            if st.button("Save to File", key="save_npc"):
                if save_character_data(
                    st.session_state.char_name,
                    character_card,
                    char_attacks.value if 'char_attacks' in locals() else "",
                    char_spells.value if 'char_spells' in locals() else ""
                ):
                    st.success("Character data saved!")
                    
            uploaded_file = st.file_uploader("Load Character", key="load_npc")
            if uploaded_file:
                data = load_character_data(uploaded_file.read())
                if data:
                    st.session_state.char_name = data['name']
                    st.session_state.npc_card = data['card']
                    st.session_state.npc_attacks = data['attacks']
                    st.session_state.npc_spells = data['spells']
                    st.rerun()

        # Player Character Card (New)
        with st.expander(f"{st.session_state.player_name}'s Character Card", expanded=True):
            player_char_card = st.text_area(
                "Player Character Description",
                value=f"""Name: {st.session_state.player_name}
Role: Warrior
Personality: Bold and determined
Background: Former guard captain
Appearance: Heavily armored with shield and sword
Notable Traits: Natural leader, protective of allies""",
                height=150,
                key="player_card"
            )
            
            # Player Character Buttons
            if st.button("Add to Message", key="add_player_card"):
                st.session_state.server_message += f"\nPlayer Character Description:\n{player_char_card}"
                st.rerun()
                
            if st.button("Save to File", key="save_player"):
                if save_character_data(
                    st.session_state.player_name,
                    player_char_card,
                    player_attacks.value if 'player_attacks' in locals() else "",
                    player_spells.value if 'player_spells' in locals() else ""
                ):
                    st.success("Character data saved!")
                    
            uploaded_file = st.file_uploader("Load Character", key="load_player")
            if uploaded_file:
                data = load_character_data(uploaded_file.read())
                if data:
                    st.session_state.player_name = data['name']
                    st.session_state.loaded_player_card = data['card']
                    st.session_state.loaded_player_attacks = data['attacks']
                    st.session_state.loaded_player_spells = data['spells']
                    st.rerun()
        
        # Scenario
        with st.expander("Scenario", expanded=True):
            scenario = st.text_area(
                "Current Scenario",
                value=f"{st.session_state.char_name} stands ready in the tavern, watching the patrons carefully.",
                height=150
            )
            if st.button("Add Scenario", use_container_width=True):
                st.session_state.server_message += f"\nCurrent Scenario:\n{scenario}"
                st.rerun()
                
        # Character States
        with st.expander(f"{st.session_state.char_name}'s States", expanded=True):
            char_states = st.text_area(
                "Character States",
                value="""Currently poisoned (3 rounds remaining)
Protected by magical barrier
Low on mana""",
                height=150
            )
            if st.button(f"Add {st.session_state.char_name}'s States", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.char_name}'s Current States:\n{char_states}"
                st.rerun()
    
    with field_col2:
        # Character Attacks
        with st.expander(f"{st.session_state.char_name}'s Attacks", expanded=True):
            char_attacks = st.text_area(
                "Available Combat Actions",
                value=st.session_state.loaded_npc_attacks,
                height=150,
                key="npc_attacks"
            )
            if st.button(f"Add {st.session_state.char_name}'s Attacks", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.char_name}'s Available Combat Actions:\n{char_attacks}"
                st.rerun()
        
        # Character Spells
        with st.expander(f"{st.session_state.char_name}'s Spells", expanded=True):
            char_spells = st.text_area(
                "Available Spells",
                value=st.session_state.loaded_npc_spells,
                height=150,
                key="npc_spells"
            )
            if st.button(f"Add {st.session_state.char_name}'s Spells", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.char_name}'s Available Spells:\n{char_spells}"
                st.rerun()
                
        # Player States
        with st.expander(f"{st.session_state.player_name}'s States", expanded=True):
            player_states = st.text_area(
                "Player States",
                value="""Stunned (1 round remaining)
Bleeding (2 damage per round)
Weakened (-2 to physical attacks)""",
                height=150
            )
            if st.button(f"Add {st.session_state.player_name}'s States", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.player_name}'s Current States:\n{player_states}"
                st.rerun()
    
    with field_col3:
        # Player Attacks
        with st.expander(f"{st.session_state.player_name}'s Attacks", expanded=True):
            player_attacks = st.text_area(
                "Player's Combat Actions",
                value=st.session_state.loaded_player_attacks,
                height=150,
                key="player_attacks"
            )
            if st.button(f"Add {st.session_state.player_name}'s Attacks", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.player_name}'s Combat Actions:\n{player_attacks}"
                st.rerun()
        
        # Player Spells
        with st.expander(f"{st.session_state.player_name}'s Spells", expanded=True):
            player_spells = st.text_area(
                "Player's Spells",
                value=st.session_state.loaded_player_spells,
                height=150,
                key="player_spells"
            )
            if st.button(f"Add {st.session_state.player_name}'s Spells", use_container_width=True):
                st.session_state.server_message += f"\n{st.session_state.player_name}'s Spells:\n{player_spells}"
                st.rerun()
        
        # Custom Field (kept for flexibility)
        with st.expander("Custom Field", expanded=True):
            custom_label = st.text_input("Field Label", "Custom Data")
            custom_data = st.text_area("Custom Data", height=150)
            if st.button("Add Custom Data", use_container_width=True):
                st.session_state.server_message += f"\n{custom_label}:\n{custom_data}"
                st.rerun()

with server_col:
    st.header("Server Communication")
    
    # Server Message Builder
    st.subheader("Message to Server")
    st.text_area("Current Message", 
                 value=replace_variables(st.session_state.server_message),
                 height=200,
                 key="message_display")
    
    # Control buttons in a row
    msg_col1, msg_col2 = st.columns(2)
    with msg_col1:
        if st.button("Clear Message", use_container_width=True):
            st.session_state.server_message = ""
            st.rerun()
    with msg_col2:
        if st.button("Send to Server", use_container_width=True):
            response = send_to_server(replace_variables(st.session_state.server_message))
            st.session_state.server_response = response
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            st.session_state.server_history.append({
                "timestamp": timestamp,
                "message": replace_variables(st.session_state.server_message),
                "response": response
            })
            st.rerun()
    
    # Server Response
    st.subheader("Server Response")
    st.text_area("Latest Response", 
                 value=st.session_state.server_response,
                 height=200,
                 key="response_display")
    
    # Response control buttons in a row
    resp_col1, resp_col2 = st.columns(2)
    with resp_col1:
        if st.button("Regenerate Response", use_container_width=True):
            response = send_to_server(replace_variables(st.session_state.server_message))
            st.session_state.server_response = response
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            st.session_state.server_history.append({
                "timestamp": timestamp,
                "message": replace_variables(st.session_state.server_message),
                "response": response
            })
            st.rerun()
    with resp_col2:
        if st.button("Add to Chat History", use_container_width=True):
            st.session_state.messages.append({
                "role": "assistant",
                "content": st.session_state.server_response
            })
            st.rerun()

# Bottom row - Server History and Chat History
st.markdown("---")
history_col, chat_col = st.columns([3, 2])

with history_col:
    # Server History Log
    hist_header_col1, hist_header_col2 = st.columns([3, 1])
    with hist_header_col1:
        st.header("Server History Log")
    with hist_header_col2:
        if st.button("Clear Server History", use_container_width=True):
            st.session_state.server_history = []
            st.rerun()
            
    for i, entry in enumerate(reversed(st.session_state.server_history)):
        with st.expander(f"Exchange {len(st.session_state.server_history) - i} - {entry['timestamp']}", expanded=False):
            st.text("Message sent:")
            st.code(entry['message'])
            st.text("Response received:")
            st.code(entry['response'])

with chat_col:
    # Chat History
    chat_header_col1, chat_header_col2 = st.columns([3, 1])
    with chat_header_col1:
        st.header("Chat History")
    with chat_header_col2:
        if st.button("Clear Chat History", use_container_width=True):
            st.session_state.messages = []
            st.rerun()
            
    chat_container = st.container()
    with chat_container:
        for message in st.session_state.messages:
            with st.chat_message(message["role"]):
                st.markdown(message["content"])