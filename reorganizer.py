# ./reorganizer.py
#!/usr/bin/env python3
import os
import shutil
from pathlib import Path

def create_directory_structure():
    """Create the base directory structure"""
    directories = [
        'ServerMessage',
        'CharacterManagement',
        'CharacterManagement/JobClassEditor',
        'CharacterManagement/RaceClassEditor',
        'DatabaseInspector',
        'SchemaManager'
    ]
    
    for directory in directories:
        Path(directory).mkdir(parents=True, exist_ok=True)
        print(f"Created directory: {directory}")

def copy_file(src, dest):
    """Copy a file and create parent directories if needed"""
    dest_path = Path(dest)
    dest_path.parent.mkdir(parents=True, exist_ok=True)
    
    try:
        shutil.copy2(src, dest)
        print(f"Copied: {src} -> {dest}")
    except FileNotFoundError:
        print(f"File not found: {src}")
    except Exception as e:
        print(f"Error copying {src}: {e}")

def organize_files():
    """Organize files into their respective directories"""
    
    # Server Message Tool files
    server_message_files = [
        ('ServerMessage.py', 'ServerMessage/ServerMessage.py'),
        ('test_api.py', 'ServerMessage/test_api.py'),
        ('requirements.txt', 'ServerMessage/requirements.txt'),
        ('ServerMessages/Instructions/OptionSelectPhase.txt', 'ServerMessage/Instructions/OptionSelectPhase.txt'),
        ('ServerMessages/Instructions/Conversation.txt', 'ServerMessage/Instructions/Conversation.txt'),
        ('ServerMessages/Instructions/AbilitySelectPhase.txt', 'ServerMessage/Instructions/AbilitySelectPhase.txt'),
        ('ServerMessages/Instructions/ConsequencesPhase.txt', 'ServerMessage/Instructions/ConsequencesPhase.txt'),
        ('ServerMessages/MessageData/MessageData.txt', 'ServerMessage/MessageData/MessageData.txt')
    ]

    # Character Management Tool files
    character_management_files = [
        ('FrontEnd/CharacterInfo/CharacterInfo.py', 'CharacterManagement/CharacterInfo.py'),
        ('FrontEnd/CharacterInfo/models/Character.py', 'CharacterManagement/models/Character.py'),
        ('FrontEnd/CharacterInfo/utils/database.py', 'CharacterManagement/utils/database.py'),
        ('FrontEnd/CharacterInfo/views/CharacterView.py', 'CharacterManagement/views/CharacterView.py'),
        ('FrontEnd/CharacterInfo/views/CharacterCreation.py', 'CharacterManagement/views/CharacterCreation.py'),
        ('FrontEnd/CharacterInfo/views/LevelUp.py', 'CharacterManagement/views/LevelUp.py'),
        ('FrontEnd/CharacterInfo/views/JobClasses/database.py', 'CharacterManagement/JobClassEditor/database.py'),
        ('FrontEnd/CharacterInfo/views/JobClasses/editor.py', 'CharacterManagement/JobClassEditor/editor.py'),
        ('FrontEnd/CharacterInfo/views/JobClasses/forms.py', 'CharacterManagement/JobClassEditor/forms.py'),
        ('FrontEnd/CharacterInfo/views/JobClasses/interface.py', 'CharacterManagement/JobClassEditor/interface.py'),
        ('FrontEnd/CharacterInfo/views/RaceEditor/interface.py', 'CharacterManagement/RaceClassEditor/interface.py'),
        ('FrontEnd/CharacterInfo/views/Shared/ClassEditor/interface.py', 'CharacterManagement/Shared/ClassEditor/interface.py'),
        ('FrontEnd/CharacterInfo/views/Shared/ClassEditor/forms.py', 'CharacterManagement/Shared/ClassEditor/forms.py')
    ]

    # Database Inspector Tool files
    database_inspector_files = [
        ('DatabaseInspector.py', 'DatabaseInspector/DatabaseInspector.py')
    ]

    # Schema Management Tool files
    schema_manager_files = [
        ('DatabaseSetup/createAbilitiesSchema.py', 'SchemaManager/schemas/createAbilitiesSchema.py'),
        ('DatabaseSetup/createCharacterSchema.py', 'SchemaManager/schemas/createCharacterSchema.py'),
        ('DatabaseSetup/createCharacterClassProgressionSchema.py', 'SchemaManager/schemas/createCharacterClassProgressionSchema.py'),
        ('DatabaseSetup/createCharacterStatsSchema.py', 'SchemaManager/schemas/createCharacterStatsSchema.py'),
        ('DatabaseSetup/createClassSchema.py', 'SchemaManager/schemas/createClassSchema.py'),
        ('DatabaseSetup/createClassCategorySchema.py', 'SchemaManager/schemas/createClassCategorySchema.py'),
        ('DatabaseSetup/createClassExclusionSchema.py', 'SchemaManager/schemas/createClassExclusionSchema.py'),
        ('DatabaseSetup/createClassSpellSchema.py', 'SchemaManager/schemas/createClassSpellSchema.py'),
        ('DatabaseSetup/createClassSubcategorySchema.py', 'SchemaManager/schemas/createClassSubcategorySchema.py'),
        ('DatabaseSetup/createClassTypeSchema.py', 'SchemaManager/schemas/createClassTypeSchema.py'),
        ('DatabaseSetup/createPrerequisiteSchema.py', 'SchemaManager/schemas/createPrerequisiteSchema.py'),
        ('DatabaseSetup/createRacesSchema.py', 'SchemaManager/schemas/createRacesSchema.py'),
        ('DatabaseSetup/createSpellSchema.py', 'SchemaManager/schemas/createSpellSchema.py'),
        ('DatabaseSetup/createStatsSchema.py', 'SchemaManager/schemas/createStatsSchema.py'),
        ('DatabaseSetup/createTalentsSchema.py', 'SchemaManager/schemas/createTalentsSchema.py'),
        ('DatabaseSetup/initializeSchema.py', 'SchemaManager/initializeSchema.py'),
        ('DatabaseSetup/TableCleanup.py', 'SchemaManager/TableCleanup.py'),
        ('DatabaseTimestampTriggers.py', 'SchemaManager/DatabaseTimestampTriggers.py')
    ]

    # Create directory structure
    create_directory_structure()

    # Copy all files
    all_files = (
        server_message_files +
        character_management_files +
        database_inspector_files +
        schema_manager_files
    )

    for src, dest in all_files:
        copy_file(src, dest)

if __name__ == "__main__":
    print("Starting file organization...")
    organize_files()
    print("\nFile organization completed!")