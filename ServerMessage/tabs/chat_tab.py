# ./ServerMessage/tabs/chat_tab.py

import streamlit as st
import requests
import json
from typing import Dict, Any

def render_chat_tab(base_url: str):
    """Handle the chat interface tab functionality"""
    st.header("Character Communication")

    # Add mode selection
    mode = st.radio(
        "Communication Mode",
        ["Chat", "Combat"],
        key="comm_mode",
        horizontal=True
    )

    # Character Status Display
    col1, col2, col3 = st.columns(3)
    with col1:
        st.subheader("Player Character")
        if 'pc_status' not in st.session_state:
            st.session_state.pc_status = "standing"
        pc_status = st.selectbox(
            "Status",
            ["standing", "prone", "flying", "swimming"],
            key="pc_status_select",
            index=["standing", "prone", "flying", "swimming"].index(st.session_state.pc_status)
        )
        st.session_state.pc_status = pc_status

    with col2:
        st.subheader("Distance")
        if 'char_distance' not in st.session_state:
            st.session_state.char_distance = 30
        distance = st.number_input(
            "Feet",
            min_value=0,
            value=st.session_state.char_distance,
            key="distance_input"
        )
        st.session_state.char_distance = distance

    with col3:
        st.subheader("NPC")
        if 'npc_status' not in st.session_state:
            st.session_state.npc_status = "standing"
        npc_status = st.selectbox(
            "Status",
            ["standing", "prone", "flying", "swimming"],
            key="npc_status_select",
            index=["standing", "prone", "flying", "swimming"].index(st.session_state.npc_status)
        )
        st.session_state.npc_status = npc_status

    # Display full chat history for reference
    st.subheader("Conversation History")
    for msg in st.session_state.chat_history:
        with st.container():
            if msg["is_user"]:
                st.markdown("**User:**")
            else:
                st.markdown("**Assistant:**")
            st.write(msg["content"])
            st.divider()

    # Display combined chat history for context tracking
    if mode == "Chat":
        st.subheader("Combined Context")
        combined_chat = ""
        total_length = 0
        
        for msg in reversed(st.session_state.chat_history):
            msg_length = len(msg["content"])
            if total_length + msg_length > st.session_state.context_length:
                break
                
            role = "User" if msg["is_user"] else "Assistant"
            new_content = f"{role}: {msg['content']}\n\n"
            combined_chat = new_content + combined_chat
            total_length += msg_length
            
        st.text_area(
            "Current Context Window",
            value=combined_chat,
            height=200,
            key="context_window",
            disabled=True
        )

    # Input fields
    st.subheader("New Message")
    instructions = st.text_area(
        "System Instructions",
        help="Specify instructions for how the model should behave and respond",
        height=100,
        key="sys_instructions"
    )

    server_message = st.text_area(
        "Message Content", 
        value="Hello!",
        help="The main message to send to the model",
        height=150,
        key="message_content"
    )

    termination = st.text_area(
        "Termination Instructions",
        help="Specify conditions or instructions for ending the response",
        height=100,
        key="termination_instructions"
    )

    if st.button("Send Message", key="send_msg_btn"):
        _handle_message_submission(
            base_url=base_url,
            instructions=instructions,
            server_message=server_message,
            termination=termination,
            mode=mode
        )

def _handle_message_submission(
    base_url: str,
    instructions: str,
    server_message: str,
    termination: str,
    mode: str
) -> None:
    """Handle the submission of a message to the server"""
    messages = []
    
    # Add system instruction if provided
    if instructions:
        messages.append({
            "role": "system",
            "content": instructions
        })
    
    # Add chat history based on context length
    if mode == "Chat":
        total_length = 0
        for msg in reversed(st.session_state.chat_history):
            msg_length = len(msg["content"])
            if total_length + msg_length > st.session_state.context_length:
                break
            messages.insert(0, {
                "role": "user" if msg["is_user"] else "assistant",
                "content": msg["content"]
            })
            total_length += msg_length
    
    # Add the current message
    messages.append({
        "role": "user",
        "content": server_message
    })

    # Construct the payload - force instruction mode
    payload = {
        "messages": messages,
        "mode": "instruct",  # Always use instruct mode
        "max_tokens": st.session_state.max_tokens,
        "temperature": st.session_state.temperature,
    }
    
    if termination:
        payload["stop"] = [termination]

    # Store payload in session state for debug
    st.session_state.last_payload = payload
    
    try:
        _send_request_to_server(base_url, payload, server_message)
    except Exception as e:
        st.error(f"Exception occurred: {e}")
        import traceback
        st.code(traceback.format_exc())

def _send_request_to_server(base_url: str, payload: Dict[str, Any], server_message: str) -> None:
    """Send the request to the server and handle the response"""
    try:
        # Test connection first
        requests.get(f"{base_url}/models", timeout=5)
        
        response = requests.post(
            f"{base_url}/chat/completions",
            json=payload,
            headers={"Content-Type": "application/json"},
            timeout=30
        )

        # Store response in session state for debug
        st.session_state.last_response = response.json() if response.status_code == 200 else {"error": response.text}

        if response.status_code == 200:
            _handle_successful_response(response, server_message)
        else:
            st.error(f"Server error: {response.status_code}\n{response.text}")
            
    except requests.exceptions.ConnectionError:
        st.error("Cannot connect to server. Please check:\n" +
                "1. Server is running\n" +
                "2. IP and port are correct\n" +
                "3. No firewall blocking connection")
    except requests.exceptions.Timeout:
        st.error("Server request timed out. The model may be busy.")
    except Exception as e:
        st.error(f"Unexpected error: {str(e)}")
        if st.session_state.get('debug_mode', False):
            import traceback
            st.code(traceback.format_exc())

def _handle_successful_response(response: requests.Response, server_message: str) -> None:
    """Handle a successful response from the server"""
    response_data = response.json()
    
    # Extract the response content
    assistant_message = response_data['choices'][0]['message']['content']
    
    # Add messages to chat history
    st.session_state.chat_history.extend([
        {"content": server_message, "is_user": True},
        {"content": assistant_message, "is_user": False}
    ])
    
    # Save chat history to local storage
    with open('chat_history.json', 'w') as f:
        json.dump(st.session_state.chat_history, f)
    
    st.success("Message sent successfully")
    st.experimental_rerun()