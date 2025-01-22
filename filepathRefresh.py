# ./filepathRefresh.py

import os
import re

def update_file_header(filepath):
    with open(filepath, 'r', encoding='utf-8') as file:
        content = file.read()
    
    relative_path = os.path.join('.', os.path.relpath(filepath))
    relative_path = relative_path.replace('\\', '/')
    new_header = f"# {relative_path}\n"
    
    if content.startswith('#'):
        lines = content.split('\n')
        if re.match(r'^# \./.*\.py$', lines[0]):
            lines[0] = new_header.rstrip()
            content = '\n'.join(lines)
        else:
            content = new_header + content
    else:
        content = new_header + content
    
    with open(filepath, 'w', encoding='utf-8') as file:
        file.write(content)

def process_directory(directory='.'):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith('.py'):
                filepath = os.path.join(root, file)
                print(f"Processing: {filepath}")
                update_file_header(filepath)

if __name__ == "__main__":
    process_directory()