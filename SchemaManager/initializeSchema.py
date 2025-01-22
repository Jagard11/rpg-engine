# ./SchemaManager/initializeSchema.py

import os
import sys
import importlib.util
import logging
from pathlib import Path

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def import_module_from_file(file_path):
    """Import a module from file path"""
    module_name = os.path.splitext(os.path.basename(file_path))[0]
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def initialize_schemas():
    """Initialize all database schemas in the correct order"""
    # Get current directory (DatabaseSetup)
    current_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Get list of all schema files in dependency order
    schema_files = [
        # First clean existing tables
        "TableCleanup.py",
        
        # Level 1: Foundation tables (no dependencies)
        "createClassTypeSchema.py",         # Class Types are used in job and race records
        "createClassCategorySchema.py",     # Class Categories are used in job, race, and spell records
        "createClassSubcategorySchema.py",  # Class subcategories are used in job, race, and spell records
        "createCharacterSchema.py",         # Characters must exist first
        "createClassSchema.py",             # Classes are referenced by many tables
        "createSpellSchema.py",             # Spells are referenced by class spell levels
        
        # Level 2: Dependent tables
        "createCharacterStatsSchema.py",           # Depends on characters
        "createPrerequisiteSchema.py",             # Depends on classes
        "createClassExclusionSchema.py",           # Depends on classes
        "createCharacterClassProgressionSchema.py", # Depends on characters and classes
        "createClassSpellSchema.py",               # Depends on characters, classes, and spells
    ]
    
    logger.info(f"Starting schema initialization with {len(schema_files)} files...")
    successful_count = 0

    # Process each schema file
    for i, schema_file in enumerate(schema_files, 1):
        logger.info(f"\n[{i}/{len(schema_files)}] Working on {schema_file}...")
        
        try:
            # Get full path to schema file
            file_path = os.path.join(current_dir, schema_file)
            if not os.path.exists(file_path):
                logger.error(f"Schema file not found: {file_path}")
                continue

            # Import the module from file path
            logger.info(f"Importing {schema_file}...")
            module = import_module_from_file(file_path)
            
            # Look for create function in various formats
            func_name = None
            if hasattr(module, 'main'):
                func_name = 'main'
            elif hasattr(module, schema_file[6:-3].lower()):  # Strip 'create' and '.py'
                func_name = schema_file[6:-3].lower()
            
            if func_name:
                logger.info(f"Running {func_name}() for {schema_file}...")
                func = getattr(module, func_name)
                func()
                successful_count += 1
                logger.info(f"Successfully completed {schema_file}")
            else:
                logger.warning(f"Warning: No entry point found for {schema_file}")

        except Exception as e:
            logger.error(f"Error in {schema_file}: {str(e)}")
            logger.error(f"Stack trace: {sys.exc_info()}")
            continue

    logger.info(f"\nSchema initialization completed. Successfully ran {successful_count}/{len(schema_files)} files.")

if __name__ == "__main__":
    initialize_schemas()