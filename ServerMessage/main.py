# ./ServerMessage/main.py

import streamlit as st
import requests
import subprocess
import sys
import os
import time
from typing import List, Dict

# Initialize session state for chat history if it doesn't exist
if 'chat_history' not in st.session_state:
    st.session_state.chat_history = []

# Determine the directory of this script (./ServerMessage)
script_dir = os.path.dirname(os.path.abspath(__file__))
# Assume the repository root is one level up from the script directory.
repo_root = os.path.abspath(os.path.join(script_dir, ".."))

st.title("Oobabooga API Client (ServerMessage)")

# --- Configure API endpoint ---
with st.sidebar:
    st.header("Server Configuration")
    ip = st.text_input("Oobabooga Server IP", value="127.0.0.1", key="server_ip")
    port = st.text_input("Oobabooga Server Port", value="5000", key="server_port")
    base_url = f"http://{ip}:{port}/v1"
    st.write("Using API Base URL:", base_url)

# Create tabs for different functionalities
chat_tab, history_tab = st.tabs(["Chat Interface", "Chat History"])

with chat_tab:
    # --- Character Communication Settings ---
    st.header("Character Communication")

    # Instructions field
    instructions = st.text_area(
        "Character Instructions",
        help="Specify instructions for how the character should behave and respond",
        height=100,
        key="char_instructions"
    )

    # Main message field
    server_message = st.text_area(
        "Message Content", 
        value="Hello!",
        help="The main message to send to the character",
        height=150,
        key="message_content"
    )

    # Termination instructions
    termination = st.text_area(
        "Termination Instructions",
        help="Specify conditions or instructions for ending the response",
        height=100,
        key="termination_instructions"
    )

    # Advanced settings in an expander
    with st.expander("Advanced Settings"):
        mode = st.radio(
            "Chat Mode",
            options=["chat", "instruct"],
            index=1,
            key="chat_mode"
        )
        
        instruction_template = st.text_input(
            "Instruction Template",
            value="Alpaca",
            help="Template to use for instruction formatting (e.g., Alpaca, Vicuna, etc.)",
            key="instruction_template"
        )
        
        temperature = st.slider(
            "Temperature",
            min_value=0.1,
            max_value=2.0,
            value=0.7,
            step=0.1,
            key="temp_slider"
        )
        max_tokens = st.number_input(
            "Max Tokens",
            min_value=1,
            max_value=2048,
            value=200,
            key="max_tokens_input"
        )

    # When the user clicks "Send Message", send an API call.
    if st.button("Send Message", key="send_msg_btn"):
        # Prepare the messages array
        messages = []
        
        # Add system instruction if provided
        if instructions:
            messages.append({
                "role": "system",
                "content": instructions
            })
        
        # Add the current message
        messages.append({
            "role": "user",
            "content": server_message
        })
        
        # Add chat history
        for msg in st.session_state.chat_history:
            messages.append({
                "role": "user" if msg["is_user"] else "assistant",
                "content": msg["content"]
            })

        # Construct the payload
        payload = {
            "messages": messages,
            "mode": mode,
            "instruction_template": instruction_template,
            "max_tokens": max_tokens,
            "temperature": temperature,
        }
        
        if termination:
            payload["stop"] = [termination]
        
        try:
            # Use the chat completions endpoint
            response = requests.post(f"{base_url}/chat/completions", json=payload)
            if response.status_code == 200:
                response_data = response.json()
                
                # Extract the response content
                assistant_message = response_data['choices'][0]['message']['content']
                
                # Add messages to chat history
                st.session_state.chat_history.extend([
                    {"content": server_message, "is_user": True},
                    {"content": assistant_message, "is_user": False}
                ])
                
                st.success("Received response from server:")
                st.write(assistant_message)
                st.divider()
                st.json(response_data)
            else:
                st.error(f"Server error: {response.status_code}\n{response.text}")
        except Exception as e:
            st.error(f"Exception occurred: {e}")

with history_tab:
    st.header("Chat History")
    
    if st.button("Clear History", key="clear_history_btn"):
        st.session_state.chat_history = []
        st.experimental_rerun()
    
    for i, message in enumerate(st.session_state.chat_history):
        with st.container():
            if message["is_user"]:
                st.markdown("**User:**")
            else:
                st.markdown("**Assistant:**")
            st.write(message["content"])
            st.divider()

# --- Git update and restart ---
st.markdown("---")
st.subheader("Update Project and Restart App")

if st.button("Update and Restart", key="update_restart_btn"):
    with st.spinner("Pulling latest code from git (repository root)..."):
        # Run "git pull" in the repository root directory.
        git_proc = subprocess.run(
            ["git", "pull"],
            cwd=repo_root,  # change directory to repository root
            capture_output=True,
            text=True
        )
        st.text("Git pull stdout:")
        st.text(git_proc.stdout)
        st.text("Git pull stderr:")
        st.text(git_proc.stderr)
        st.success("Git update complete.")
    
    st.info("Restarting Streamlit app to pick up changes...")
    time.sleep(1)
    # Change the working directory to the script's directory (./ServerMessage)
    os.chdir(script_dir)
    # Explicitly define the absolute path to the script.
    new_script = os.path.join(script_dir, "main.py")
    # Restart using "python -m streamlit run ..." so that Streamlit sets up its context.
    os.execv(
        sys.executable,
        [sys.executable, "-m", "streamlit", "run", new_script] + sys.argv[1:]
    )