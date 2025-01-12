import streamlit as st
import requests
from datetime import datetime

# Initialize session state variables
if 'server_response' not in st.session_state:
    st.session_state.server_response = ""
if 'server_address' not in st.session_state:
    st.session_state.server_address = "http://127.0.0.1:5000/v1/chat/completions"

def send_to_server(message):
    """Send message to the API and return the response"""
    URL = st.session_state.server_address
    
    messages = [{"role": "user", "content": message}]
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

# Streamlit UI
st.title("Simple Server Communication")

# Server address input
st.subheader("Server Configuration")
server_address = st.text_input(
    "Server address",
    value=st.session_state.server_address,
    help="Enter the full URL of the server endpoint"
)
# Update session state when address changes
if server_address != st.session_state.server_address:
    st.session_state.server_address = server_address

# Instruction field
instruction = st.text_area("Instruction", 
                          value="Enter your instruction here",
                          height=100)

# Message to server
st.subheader("Message to Server")
message = st.text_area("Enter your message",
                      height=200)

# Send button
if st.button("Send to Server"):
    full_message = f"{instruction}\n\n{message}" if instruction.strip() else message
    response = send_to_server(full_message)
    st.session_state.server_response = response

# Server Response
st.subheader("Server Response")
st.text_area("Latest Response",
             value=st.session_state.server_response,
             height=200)