import streamlit as st
import requests
from datetime import datetime

# Configure Streamlit page to use wide mode
st.set_page_config(layout="wide")

# Initialize ALL session state variables first
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
    
if 'instruction' not in st.session_state:
    st.session_state.instruction = "Choose how to respond to the player"
    
if 'message_sections' not in st.session_state:
    st.session_state.message_sections = {}

# Initialize all toggle states
if 'toggle_instruction' not in st.session_state:
    st.session_state.toggle_instruction = True
    
if 'toggle_npc_card' not in st.session_state:
    st.session_state.toggle_npc_card = False
    
if 'toggle_char_stats' not in st.session_state:
    st.session_state.toggle_char_stats = False
    
if 'toggle_player_stats' not in st.session_state:
    st.session_state.toggle_player_stats = False
    
if 'toggle_chat_log' not in st.session_state:
    st.session_state.toggle_chat_log = False
    
if 'toggle_combat_log' not in st.session_state:
    st.session_state.toggle_combat_log = False
    
if 'toggle_react_to' not in st.session_state:
    st.session_state.toggle_react_to = False
    
if 'toggle_player_card' not in st.session_state:
    st.session_state.toggle_player_card = False
    
if 'toggle_scenario' not in st.session_state:
    st.session_state.toggle_scenario = False
    
if 'toggle_char_states' not in st.session_state:
    st.session_state.toggle_char_states = False
    
if 'toggle_char_attacks' not in st.session_state:
    st.session_state.toggle_char_attacks = False
    
if 'toggle_char_spells' not in st.session_state:
    st.session_state.toggle_char_spells = False
    
if 'toggle_player_states' not in st.session_state:
    st.session_state.toggle_player_states = False
    
if 'toggle_player_attacks' not in st.session_state:
    st.session_state.toggle_player_attacks = False
    
if 'toggle_player_spells' not in st.session_state:
    st.session_state.toggle_player_spells = False
    
if 'toggle_custom_field' not in st.session_state:
    st.session_state.toggle_custom_field = False

# Initialize any other required session state variables

if 'forced_response' not in st.session_state:
    st.session_state.forced_response = ""
    
if 'message_sections' not in st.session_state:
    st.session_state.message_sections = {}
    
# Initialize default content if not in session state
if 'loaded_npc_card' not in st.session_state:
    st.session_state.loaded_npc_card = f"""Name: {st.session_state.char_name}
Role: Battle Mage
Personality: Scholarly and curious
Background: Trained at the Academy of High Magic
Appearance: Robed figure with glowing staff
Notable Traits: Speaks in a formal manner, values knowledge"""

if 'char_stats' not in st.session_state:
    st.session_state.char_stats = "HP: 10/10\nMP: 8/8"
    
if 'player_stats' not in st.session_state:
    st.session_state.player_stats = "HP: 15/15\nMP: 5/5"
    
if 'chat_log' not in st.session_state:
    st.session_state.chat_log = ""
    
if 'combat_log' not in st.session_state:
    st.session_state.combat_log = ""
    
if 'react_to' not in st.session_state:
    st.session_state.react_to = ""

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

def send_to_server(message, forced_response=None):
    """Send message to the API and return the response"""
    URL = "http://127.0.0.1:5000/v1/chat/completions"
    
    # If there's a forced response, prepend it to the message with a special marker
    if forced_response and forced_response.strip():
        # Create system message to force the response using proper API instruction format
        system_message = {"role": "system", "content": "You are an assistant designed to generate text. You need to follow the rules and constraints given to you to generate an appropriate response."}
        format_message = {"role": "user", "content": f"Please complete the following response, starting exactly with: {forced_response}"}
        main_message = {"role": "user", "content": replace_variables(message)}
        messages = [system_message, format_message, main_message]
    else:
        messages = [{"role": "user", "content": replace_variables(message)}]
    
    request_data = {
        "messages": messages,
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

# Top section - Instruction and names
instruction_text = st.text_area("Instruction", 
             value=st.session_state.instruction, 
             height=100,
             key="instruction")

if st.toggle('Include Instruction', key='toggle_instruction', value=st.session_state.toggle_instruction):
    if 'instruction_content' not in st.session_state.message_sections:
        st.session_state.message_sections['instruction_content'] = instruction_text
else:
    if 'instruction_content' in st.session_state.message_sections:
        del st.session_state.message_sections['instruction_content']

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
            
            # Initialize toggle state if not exists
            if 'toggle_npc_card' not in st.session_state:
                st.session_state.toggle_npc_card = False
                
            # Add toggle for character card
            if st.toggle('Include in Message', key='toggle_npc_card', value=st.session_state.toggle_npc_card):
                if 'npc_card_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['npc_card_content'] = f"\nCharacter Description:\n{character_card}"
            else:
                if 'npc_card_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['npc_card_content']
            
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
            
            # Initialize toggle state if not exists
            if 'toggle_player_card' not in st.session_state:
                st.session_state.toggle_player_card = False
                
            # Add toggle for player character card
            if st.toggle('Include in Message', key='toggle_player_card', value=st.session_state.toggle_player_card):
                if 'player_card_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['player_card_content'] = f"\nPlayer Character Description:\n{player_char_card}"
            else:
                if 'player_card_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['player_card_content']
            
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
            
            # Initialize toggle state if not exists
            if 'toggle_scenario' not in st.session_state:
                st.session_state.toggle_scenario = False
                
            if st.toggle('Include in Message', key='toggle_scenario', value=st.session_state.toggle_scenario):
                if 'scenario_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['scenario_content'] = f"\nCurrent Scenario:\n{scenario}"
            else:
                if 'scenario_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['scenario_content']
                
        # Character Stats
        with st.expander(f"{st.session_state.char_name}'s Stats", expanded=True):
            char_stats = st.text_area(
                "Character Stats",
                value=st.session_state.char_stats,
                height=100,
                key="char_stats"
            )
            
            if st.toggle('Include in Message', key='toggle_char_stats', value=st.session_state.toggle_char_stats):
                if 'char_stats_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['char_stats_content'] = f"\n{st.session_state.char_name}'s Stats:\n{char_stats}"
            else:
                if 'char_stats_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['char_stats_content']

        # Player Stats
        with st.expander(f"{st.session_state.player_name}'s Stats", expanded=True):
            player_stats = st.text_area(
                "Player Stats",
                value=st.session_state.player_stats,
                height=100,
                key="player_stats"
            )
            
            if st.toggle('Include in Message', key='toggle_player_stats', value=st.session_state.toggle_player_stats):
                if 'player_stats_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['player_stats_content'] = f"\n{st.session_state.player_name}'s Stats:\n{player_stats}"
            else:
                if 'player_stats_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['player_stats_content']

        # Chat Log
        with st.expander("Chat Log", expanded=True):
            chat_log = st.text_area(
                "Recent Chat History",
                value=st.session_state.chat_log,
                height=150,
                key="chat_log"
            )
            
            if st.toggle('Include in Message', key='toggle_chat_log', value=st.session_state.toggle_chat_log):
                if 'chat_log_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['chat_log_content'] = f"\n<CHAT_LOG>\n{chat_log}\n</CHAT_LOG>"
            else:
                if 'chat_log_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['chat_log_content']

        # Combat Log
        with st.expander("Combat Log", expanded=True):
            combat_log = st.text_area(
                "Recent Combat Actions",
                value=st.session_state.combat_log,
                height=150,
                key="combat_log"
            )
            
            if st.toggle('Include in Message', key='toggle_combat_log', value=st.session_state.toggle_combat_log):
                if 'combat_log_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['combat_log_content'] = f"\n<COMBAT_LOG>\n{combat_log}\n</COMBAT_LOG>"
            else:
                if 'combat_log_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['combat_log_content']

        # React To
        with st.expander("React To", expanded=True):
            react_to = st.text_area(
                "Content to React To",
                value=st.session_state.react_to,
                height=150,
                key="react_to"
            )
            
            if st.toggle('Include in Message', key='toggle_react_to', value=st.session_state.toggle_react_to):
                if 'react_to_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['react_to_content'] = f"\n<REACT_TO>\n{react_to}\n</REACT_TO>"
            else:
                if 'react_to_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['react_to_content']

        # Character States
        with st.expander(f"{st.session_state.char_name}'s States", expanded=True):
            char_states = st.text_area(
                "Character States",
                value="""Currently poisoned (3 rounds remaining)
Protected by magical barrier
Low on mana""",
                height=150
            )
            
            # Initialize toggle state if not exists
            if 'toggle_char_states' not in st.session_state:
                st.session_state.toggle_char_states = False
                
            if st.toggle('Include in Message', key='toggle_char_states', value=st.session_state.toggle_char_states):
                if 'char_states_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['char_states_content'] = f"\n{st.session_state.char_name}'s Current States:\n{char_states}"
            else:
                if 'char_states_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['char_states_content']
    
    with field_col2:
        # Character Attacks
        with st.expander(f"{st.session_state.char_name}'s Attacks", expanded=True):
            char_attacks = st.text_area(
                "Available Combat Actions",
                value=st.session_state.loaded_npc_attacks,
                height=150,
                key="npc_attacks"
            )
            # Initialize toggle state if not exists
            if 'toggle_char_attacks' not in st.session_state:
                st.session_state.toggle_char_attacks = False
                
            if st.toggle('Include in Message', key='toggle_char_attacks', value=st.session_state.toggle_char_attacks):
                if 'char_attacks_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['char_attacks_content'] = f"\n{st.session_state.char_name}'s Available Combat Actions:\n{char_attacks}"
            else:
                if 'char_attacks_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['char_attacks_content']
        
        # Character Spells
        with st.expander(f"{st.session_state.char_name}'s Spells", expanded=True):
            char_spells = st.text_area(
                "Available Spells",
                value=st.session_state.loaded_npc_spells,
                height=150,
                key="npc_spells"
            )
            
            # Initialize toggle state if not exists
            if 'toggle_char_spells' not in st.session_state:
                st.session_state.toggle_char_spells = False
                
            if st.toggle('Include in Message', key='toggle_char_spells', value=st.session_state.toggle_char_spells):
                if 'char_spells_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['char_spells_content'] = f"\n{st.session_state.char_name}'s Available Spells:\n{char_spells}"
            else:
                if 'char_spells_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['char_spells_content']
                    
        # Player States
        with st.expander(f"{st.session_state.player_name}'s States", expanded=True):
            player_states = st.text_area(
                "Player States",
                value="""Stunned (1 round remaining)
Bleeding (2 damage per round)
Weakened (-2 to physical attacks)""",
                height=150
            )
            
            # Initialize toggle state if not exists
            if 'toggle_player_states' not in st.session_state:
                st.session_state.toggle_player_states = False
                
            if st.toggle('Include in Message', key='toggle_player_states', value=st.session_state.toggle_player_states):
                if 'player_states_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['player_states_content'] = f"\n{st.session_state.player_name}'s Current States:\n{player_states}"
            else:
                if 'player_states_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['player_states_content']
    
    with field_col3:
        # Player Attacks
        with st.expander(f"{st.session_state.player_name}'s Attacks", expanded=True):
            player_attacks = st.text_area(
                "Player's Combat Actions",
                value=st.session_state.loaded_player_attacks,
                height=150,
                key="player_attacks"
            )
            
            # Initialize toggle state if not exists
            if 'toggle_player_attacks' not in st.session_state:
                st.session_state.toggle_player_attacks = False
                
            if st.toggle('Include in Message', key='toggle_player_attacks', value=st.session_state.toggle_player_attacks):
                if 'player_attacks_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['player_attacks_content'] = f"\n{st.session_state.player_name}'s Combat Actions:\n{player_attacks}"
            else:
                if 'player_attacks_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['player_attacks_content']
        
        # Player Spells
        with st.expander(f"{st.session_state.player_name}'s Spells", expanded=True):
            player_spells = st.text_area(
                "Player's Spells",
                value=st.session_state.loaded_player_spells,
                height=150,
                key="player_spells"
            )
            
            # Initialize toggle state if not exists
            if 'toggle_player_spells' not in st.session_state:
                st.session_state.toggle_player_spells = False
                
            if st.toggle('Include in Message', key='toggle_player_spells', value=st.session_state.toggle_player_spells):
                if 'player_spells_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['player_spells_content'] = f"\n{st.session_state.player_name}'s Spells:\n{player_spells}"
            else:
                if 'player_spells_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['player_spells_content']
        
        # Custom Field
        with st.expander("Custom Field", expanded=True):
            custom_label = st.text_input("Field Label", "Custom Data")
            custom_data = st.text_area("Custom Data", height=150)
            
            # Initialize toggle state if not exists
            if 'toggle_custom_field' not in st.session_state:
                st.session_state.toggle_custom_field = False
                
            if st.toggle('Include in Message', key='toggle_custom_field', value=st.session_state.toggle_custom_field):
                if 'custom_field_content' not in st.session_state.message_sections:
                    st.session_state.message_sections['custom_field_content'] = f"\n{custom_label}:\n{custom_data}"
            else:
                if 'custom_field_content' in st.session_state.message_sections:
                    del st.session_state.message_sections['custom_field_content']

with server_col:
    st.header("Server Communication")
    
    # Server Message Builder
    st.subheader("Message to Server")
    # Combine all active sections for display
    combined_message = "\n".join(st.session_state.message_sections.values())
    st.text_area("Current Message", 
                 value=replace_variables(combined_message),
                 height=200,
                 key="message_display")
    
    # Control buttons in a row
    msg_col1, msg_col2 = st.columns(2)
    with msg_col1:
        if st.button("Clear Message", use_container_width=True):
            st.session_state.message_sections = {}
            for key in list(st.session_state.keys()):
                if key.startswith('toggle_'):
                    st.session_state[key] = False
            st.rerun()
    with msg_col2:
        if st.button("Send to Server", use_container_width=True):
            # Get instruction if toggled on
            instruction = st.session_state.message_sections.get('instruction_content', '')
            
            # Combine message sections (excluding instruction)
            message_sections = {k: v for k, v in st.session_state.message_sections.items() 
                              if k != 'instruction_content'}
            message_content = "\n".join(message_sections.values())
            
            # Build full message based on whether instruction is included
            if instruction:
                full_message = f"{instruction}\n\n{message_content}"
            else:
                full_message = message_content
            
            # Get forced response if any
            forced_response = st.session_state.get('forced_response', '').strip()
            
            response = send_to_server(full_message, forced_response)
            st.session_state.server_response = response
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            st.session_state.server_history.append({
                "timestamp": timestamp,
                "message": full_message,
                "response": response,
                "forced_response": forced_response if forced_response else None
            })
            st.rerun()
    
    # Forced Response field
    st.subheader("Forced Response")
    st.text_area("Force the server response to begin with:", 
                 value=st.session_state.get('forced_response', ''),
                 height=100,
                 key='forced_response',
                 help="If not empty, the server will be forced to start its response with this exact text.")
    
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
            if entry.get('forced_response'):
                st.text("Forced response:")
                st.code(entry['forced_response'])
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