# ./ServerMessage/tabs/git_tab.py

import streamlit as st
import subprocess
import sys
import os
import time

def render_git_tab(script_dir: str, repo_root: str):
    """
    Render the git updates tab
    
    Args:
        script_dir (str): The directory containing the script
        repo_root (str): The root directory of the git repository
    """
    st.header("Update Project")
    st.write("Update the project code from the git repository and restart the application.")

    # Show previous git results if they exist
    if st.session_state.show_git_results and st.session_state.git_output:
        st.success("Previous Git Update Results:")
        st.text("Git pull stdout:")
        st.text(st.session_state.git_output["stdout"])
        st.text("Git pull stderr:")
        st.text(st.session_state.git_output["stderr"])
    
    if st.button("Update and Restart", key="update_restart_btn"):
        with st.spinner("Pulling latest code from git (repository root)..."):
            # Run "git pull" in the repository root directory
            git_proc = subprocess.run(
                ["git", "pull"],
                cwd=repo_root,
                capture_output=True,
                text=True
            )
            
            # Store the output in session state
            st.session_state.git_output = {
                "stdout": git_proc.stdout,
                "stderr": git_proc.stderr
            }
            st.session_state.show_git_results = True
            
            # Display the current results
            st.success("Git Update Results:")
            st.text("Git pull stdout:")
            st.text(git_proc.stdout)
            st.text("Git pull stderr:")
            st.text(git_proc.stderr)
            
            st.success("Git update complete.")
            st.info("The application will restart in 5 seconds...")
            time.sleep(5)  # Give time to read the output
            
            # Restart the application
            os.chdir(script_dir)
            new_script = os.path.join(script_dir, "main.py")
            os.execv(
                sys.executable,
                [sys.executable, "-m", "streamlit", "run", new_script] + sys.argv[1:]
            )