# ./DatabaseSetup/initializeSchema.py

import os
import sys
from importlib import import_module
from pathlib import Path

def run_initialization():
    # Files to run in order
    schema_files = [
        'TableCleanup',
        'createCharacterSchema',
        'createSpellSchema',
        'createClassSystemSchema'
    ]
    
    # Add parent directory to path for imports
    current_dir = Path(__file__).parent
    parent_dir = current_dir.parent
    sys.path.append(str(parent_dir))
    
    total_files = len(schema_files)
    completed_files = 0
    
    print(f"Starting schema initialization with {total_files} files...")
    
    for schema_file in schema_files:
        print(f"\n[{completed_files + 1}/{total_files}] Working on {schema_file}...")
        try:
            if schema_file == 'createClassSystemSchema':
                print("Initializing class system...")
                # Special handling for class system which uses a class
                module = import_module(f'DatabaseSetup.{schema_file}')
                initializer = module.ClassSystemInitializer()
                initializer.setup_class_system()
            else:
                # Import and run the module
                print(f"Importing {schema_file}...")
                module = import_module(f'DatabaseSetup.{schema_file}')
                
                # Try different function naming patterns
                if hasattr(module, 'main'):
                    print(f"Running main() for {schema_file}...")
                    module.main()
                elif hasattr(module, schema_file):
                    print(f"Running {schema_file}() function...")
                    getattr(module, schema_file)()
                elif hasattr(module, schema_file.lower()):
                    print(f"Running {schema_file.lower()}() function...")
                    getattr(module, schema_file.lower())()
                else:
                    print(f"Warning: No entry point found for {schema_file}")
                    continue
            
            print(f"Successfully completed {schema_file}")
            completed_files += 1
            
        except Exception as e:
            print(f"Error in {schema_file}: {str(e)}")
            print(f"Stack trace:", sys.exc_info())
            continue
    
    print(f"\nSchema initialization completed. Successfully ran {completed_files}/{total_files} files.")

if __name__ == "__main__":
    run_initialization()