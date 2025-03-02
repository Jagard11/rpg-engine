# ./compile_project_files.py
import os
import logging

# Configure logging to display in the terminal
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def compile_project_files(root_dir, file_extensions=['.py']):
    """
    Compiles all files with the specified extensions from each directory and its subdirectories
    into separate output files, named Compiled_<directoryname>.txt, placed within their 
    respective directories.

    Args:
        root_dir (str): The root directory of the project.
        file_extensions (list): List of file extensions to include (default is ['.py']).
    """
    # Walk through the directory tree
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # Skip the root directory itself to avoid creating a compiled file there
        if dirpath == root_dir:
            continue
        
        # Get the directory name relative to root_dir
        directory_name = os.path.relpath(dirpath, root_dir).replace(os.sep, '_')
        # Define output file name and path within the current directory
        output_file = f"Compiled_{directory_name}.txt"
        output_path = os.path.join(dirpath, output_file)
        
        # Flag to check if anything was written
        content_written = False
        
        # Open the output file for writing within the current directory
        with open(output_path, 'w', encoding='utf-8') as outfile:
            # Walk through this directory and all its subdirectories
            for sub_dirpath, _, sub_filenames in os.walk(dirpath):
                # Filter files with specified extensions
                valid_files = [f for f in sub_filenames if any(f.endswith(ext) for ext in file_extensions)]
                
                if valid_files:
                    # Log the directory being processed and valid file count
                    if sub_dirpath == dirpath:
                        logging.info(f"Processing directory and its subdirectories: {os.path.relpath(dirpath, root_dir)}")
                    total_valid_files = len(valid_files)
                    logging.info(f"Found {total_valid_files} valid files in {os.path.relpath(sub_dirpath, root_dir)}")
                    
                    # Write files in sorted order
                    for filename in sorted(valid_files):
                        file_path = os.path.join(sub_dirpath, filename)
                        relative_path = os.path.relpath(file_path, root_dir)
                        
                        # Write header with file path and name
                        outfile.write(f"\n{'='*80}\n")
                        outfile.write(f"File: {relative_path}\n")
                        outfile.write(f"{'='*80}\n\n")
                        
                        # Write file content
                        with open(file_path, 'r', encoding='utf-8') as infile:
                            outfile.write(infile.read())
                        outfile.write("\n")  # Add a newline for separation
                        content_written = True
        
        # Handle the output file based on whether content was written
        if not content_written and os.path.exists(output_path):
            os.remove(output_path)
            logging.info(f"No compiled file generated for {os.path.relpath(dirpath, root_dir)} (no valid content)")
        elif content_written:
            logging.info(f"Generated compiled file: {os.path.relpath(output_path, root_dir)}")
        else:
            logging.info(f"No compiled file generated for {os.path.relpath(dirpath, root_dir)} (unexpected case)")

if __name__ == "__main__":
    # Set the root directory to the current working directory (project root)
    root_directory = os.getcwd()
    
    # List of file extensions to process
    extensions_to_compile = ['.py', '.cpp', '.hpp']
    
    # Compile files for all specified extensions
    compile_project_files(root_directory, file_extensions=extensions_to_compile)
    
    logging.info("Compilation complete. Check each subdirectory for Compiled_<directoryname>.txt files.")