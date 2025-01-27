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

def find_create_function(module, filename):
    """Find the create function in the module using various naming patterns"""
    # Common function names to check
    candidates = [
        'main',
        'create_schema',
        filename[6:-3].lower(),  # Strip 'create' and '.py'
    ]
    
    # Add specific schema creation function name
    base_name = filename[6:-9].lower()  # Remove 'create' and 'Schema.py'
    candidates.append(f'create_{base_name}_schema')
    
    # Check each candidate
    for func_name in candidates:
        if hasattr(module, func_name):
            return getattr(module, func_name)
    
    return None

def initialize_schemas():
    """Initialize all database schemas in the correct order"""
    # Get current directory (DatabaseSetup)
    current_dir = os.path.dirname(os.path.abspath(__file__))
    schemas_dir = os.path.join(current_dir, 'schemas')

    
    # Get list of all schema files in dependency order
    schema_files = [
        # First clean existing tables
        "../TableCleanup.py",
        
        # Level 1: Foundation tables (no dependencies)
        "createClassTypesSchema.py",         # Class Types are used in job and race records
        "createClassCategoriesSchema.py",     # Class Categories are used in job, race, and spell records
        "createClassSubcategoriesSchema.py",  # Class subcategories are used in job, race, and spell records
        "createCharactersSchema.py",         # Characters must exist first
        "createClassesSchema.py",             # Classes are referenced by many tables
        "createSpellsSchema.py",             # Spells are referenced by class spell levels
        
        # Level 2: Dependent tables
        "createCharacterStatsSchema.py",           # Depends on characters
        "createClassPrerequisitesSchema.py",             # Depends on classes
        "createClassExclusionsSchema.py",           # Depends on classes
        "createCharacterClassProgressionSchema.py", # Depends on characters and classes
        "createClassSpellLevelsSchema.py",               # Depends on characters, classes, and spells
    
        # Misc
        "createClassPrerequisitesSchema.py",
        "createSpellTiersSchema.py",
        "createSpellTypeSchema.py",

    ]
    
    logger.info(f"Starting schema initialization with {len(schema_files)} files...")
    successful_count = 0

    # Process each schema file
    for i, schema_file in enumerate(schema_files, 1):
        logger.info(f"\n[{i}/{len(schema_files)}] Working on {schema_file}...")
        
        try:
            # Get full path to schema file
            file_path = os.path.join(schemas_dir, schema_file)
            if not os.path.exists(file_path):
                logger.error(f"Schema file not found: {file_path}")
                continue

            # Import the module from file path
            logger.info(f"Importing {schema_file}...")
            module = import_module_from_file(file_path)
            
            create_func = find_create_function(module, schema_file)
            
            if create_func:
                logger.info(f"Running {create_func.__name__}() for {schema_file}...")
                create_func()
                successful_count += 1
                logger.info(f"Successfully completed {schema_file}")
            else:
                logger.warning(f"Warning: No create function found for {schema_file}")

        except Exception as e:
            logger.error(f"Error in {schema_file}: {str(e)}")
            logger.error(f"Stack trace: {sys.exc_info()}")
            continue

    logger.info(f"\nSchema initialization completed. Successfully ran {successful_count}/{len(schema_files)} files.")

if __name__ == "__main__":
    initialize_schemas()