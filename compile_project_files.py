# ./compile_project_files.py

import os

def compile_project_files(root_dir, file_extension='.py'):
    """
    Compiles all files with the specified extension in each subdirectory into separate output files,
    named Compiled_<subdirectoryname>.txt.

    Args:
        root_dir (str): The root directory of the project.
        file_extension (str): The file extension to include (default is '.py' for Python files).
    """
    # Iterate over immediate subdirectories in the root
    for subdirectory in os.listdir(root_dir):
        subdir_path = os.path.join(root_dir, subdirectory)
        if os.path.isdir(subdir_path):
            # Define output file name for this subdirectory
            output_file = f"Compiled_{subdirectory}.txt"
            output_path = os.path.join(root_dir, output_file)
            
            # Open the output file for writing
            with open(output_path, 'w', encoding='utf-8') as outfile:
                # Walk through this subdirectory only
                for dirpath, _, filenames in os.walk(subdir_path):
                    for filename in sorted(filenames):  # Sort for consistent order
                        if filename.endswith(file_extension):
                            file_path = os.path.join(dirpath, filename)
                            relative_path = os.path.relpath(file_path, root_dir)
                            
                            # Write header with file path and name
                            outfile.write(f"\n{'='*80}\n")
                            outfile.write(f"File: {relative_path}\n")
                            outfile.write(f"{'='*80}\n\n")
                            
                            # Write file content
                            with open(file_path, 'r', encoding='utf-8') as infile:
                                outfile.write(infile.read())
                            outfile.write("\n")  # Add a newline for separation
            
            # Check if anything was written to the file; if not, remove it
            if os.path.exists(output_path) and os.path.getsize(output_path) == 0:
                os.remove(output_path)
            else:
                print(f"Compiled files from {subdirectory} into {output_file}")

if __name__ == "__main__":
    # Set the root directory to the current working directory (project root)
    root_directory = os.getcwd()
    
    # Compile all .py files
    compile_project_files(root_directory)
    
    print("Compilation complete. Check the root directory for Compiled_<subdirectoryname>.txt files.")