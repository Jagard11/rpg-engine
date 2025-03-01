# ./filepathRefresh.py

import os
import re

def update_file_header(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as file:
            content = file.read()
        
        relative_path = os.path.join('.', os.path.relpath(filepath))
        relative_path = relative_path.replace('\\', '/')
        
        # Determine file extension and set appropriate comment style
        file_extension = os.path.splitext(filepath)[1]
        if file_extension == '.py':
            new_header = f"# {relative_path}\n"
            comment_pattern = r'^# \./.*\.py$'
        elif file_extension in ('.cpp', '.hpp'):
            new_header = f"// {relative_path}\n"
            comment_pattern = r'^// \./.*\.(cpp|hpp)$'
        
        if content.startswith(('#', '//')):
            lines = content.split('\n')
            if re.match(comment_pattern, lines[0]):
                lines[0] = new_header.rstrip()
                content = '\n'.join(lines)
            else:
                content = new_header + content
        else:
            content = new_header + content
        
        with open(filepath, 'w', encoding='utf-8') as file:
            file.write(content)
            
        # Print success message immediately after processing
        print(f"Successfully processed: {relative_path}")
        
        return True
    except Exception as e:
        print(f"Error processing {relative_path}: {str(e)}")
        return False

def process_directory(directory='.'):
    processed_files = 0
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.py', '.cpp', '.hpp')):
                filepath = os.path.join(root, file)
                print(f"Attempting to process: {filepath}")
                if update_file_header(filepath):
                    processed_files += 1
    
    # Summary at the end
    print(f"\nProcessing complete. Total files successfully processed: {processed_files}")

if __name__ == "__main__":
    process_directory()