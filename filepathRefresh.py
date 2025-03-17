#!/usr/bin/env python3

import os
import re
import argparse
import sys

def update_file_header(filepath, llm_arena_dir=None):
    """
    Update file header with relative path, considering nested directory structure
    
    :param filepath: Full path to the file being processed
    :param llm_arena_dir: Root LLMArena directory to compute relative path
    :return: True if successful, False otherwise
    """
    try:
        with open(filepath, 'r', encoding='utf-8') as file:
            content = file.read()
        
        # Determine the most appropriate relative path
        if llm_arena_dir:
            # Try to compute relative path from the LLMArena directory
            try:
                relative_path = os.path.relpath(filepath, llm_arena_dir)
            except ValueError:
                # Fallback to standard relative path
                relative_path = os.path.relpath(filepath)
        else:
            relative_path = os.path.relpath(filepath)
        
        # Normalize path separators
        relative_path = relative_path.replace('\\', '/')
        
        # Determine file extension and set appropriate comment style
        file_extension = os.path.splitext(filepath)[1]
        if file_extension == '.py':
            new_header = f"# {relative_path}\n"
            comment_pattern = r'^# .*\.(py|h|cpp)$'
        elif file_extension in ('.cpp', '.hpp', '.h'):
            new_header = f"// {relative_path}\n"
            comment_pattern = r'^// .*\.(py|h|cpp)$'
        else:
            return False  # Skip unsupported file types
        
        # Check and update or add header
        if content.startswith(('#', '//')):
            lines = content.split('\n')
            if re.match(comment_pattern, lines[0]):
                # Replace existing header
                lines[0] = new_header.rstrip()
                content = '\n'.join(lines)
            else:
                # Prepend new header
                content = new_header + content
        else:
            # Add header to beginning of file
            content = new_header + content
        
        with open(filepath, 'w', encoding='utf-8') as file:
            file.write(content)
            
        # Print success message immediately after processing
        print(f"Successfully processed: {relative_path}")
        
        return True
    except Exception as e:
        print(f"Error processing {filepath}: {str(e)}")
        return False

def find_llm_arena_dir(start_dir='.'):
    """
    Find the LLMArena directory from the given starting directory.
    Checks multiple possible locations and scenarios.
    
    :param start_dir: Directory to start searching from
    :return: Full path to LLMArena directory or None
    """
    # Possible search paths
    search_paths = [
        start_dir,  # Current directory
        os.path.dirname(start_dir),  # Parent directory
        os.path.dirname(os.path.abspath(__file__)),  # Script directory
        os.path.join(os.path.dirname(os.path.abspath(__file__)), 'LLMArena'),
        os.path.join(start_dir, 'LLMArena'),
        os.path.join(os.path.dirname(start_dir), 'LLMArena')
    ]
    
    # Remove duplicates while preserving order
    search_paths = list(dict.fromkeys(search_paths))
    
    for path in search_paths:
        # Check if this path is or contains the LLMArena directory
        if os.path.basename(path) == 'LLMArena' and os.path.isdir(path):
            return path
        
        # Check for subdirectories that might be LLMArena
        llm_arena_candidates = [
            path,
            os.path.join(path, 'LLMArena'),
            os.path.join(path, 'llm_arena'),
            os.path.join(path, 'llmarena')
        ]
        
        for candidate in llm_arena_candidates:
            if os.path.exists(candidate) and os.path.isdir(candidate):
                # Additional validation: check if it has common LLMArena directory structure
                common_subdirs = ['include', 'src', 'resources']
                if any(os.path.exists(os.path.join(candidate, subdir)) for subdir in common_subdirs):
                    return candidate
    
    return None

def process_directory(directory='.', llm_arena_mode=False):
    """
    Process files in a directory, optionally focusing on LLMArena project
    
    :param directory: Starting directory to process
    :param llm_arena_mode: Whether to focus on LLMArena project
    """
    processed_files = 0
    
    if llm_arena_mode:
        # Find the LLMArena directory
        llm_arena_dir = find_llm_arena_dir(directory)
        
        if not llm_arena_dir:
            print(f"Error: LLMArena directory not found. Checked in and around {os.path.abspath(directory)}")
            return
        
        print(f"Processing files in LLMArena directory: {llm_arena_dir}")
        
        # Process files recursively in the LLMArena directory
        for root, _, files in os.walk(llm_arena_dir):
            for file in files:
                if file.endswith(('.py', '.cpp', '.hpp', '.h')):
                    filepath = os.path.join(root, file)
                    if update_file_header(filepath, llm_arena_dir):
                        processed_files += 1
    else:
        # Original behavior - process all files recursively
        for root, _, files in os.walk(directory):
            for file in files:
                if file.endswith(('.py', '.cpp', '.hpp', '.h')):
                    filepath = os.path.join(root, file)
                    if update_file_header(filepath):
                        processed_files += 1
    
    # Summary at the end
    print(f"\nProcessing complete. Total files successfully processed: {processed_files}")

def show_menu():
    """
    Display interactive menu for the script
    """
    print("\nfilepathRefresh.py - File Header Updater")
    print("----------------------------------------")
    print("1. Run default (process all files recursively)")
    print("2. Run in LLMArena mode (process only LLMArena project files)")
    print("0. Exit")
    
    choice = input("\nEnter your choice (0-2): ")
    
    if choice == '1':
        process_directory()
    elif choice == '2':
        process_directory('.', True)
    elif choice == '0':
        print("Exiting program.")
        sys.exit(0)
    else:
        print("Invalid choice. Please try again.")
        show_menu()

def main():
    """
    Main entry point of the script
    """
    parser = argparse.ArgumentParser(description='Update file headers with relative paths.')
    parser.add_argument('--mode', choices=['default', 'llm_arena'], 
                        help='Execution mode: default (process all files) or llm_arena (process only LLMArena files)')
    
    args = parser.parse_args()
    
    if args.mode:
        if args.mode == 'default':
            process_directory()
        elif args.mode == 'llm_arena':
            process_directory('.', True)
    else:
        # Show interactive menu if no arguments provided
        show_menu()

if __name__ == "__main__":
    main()