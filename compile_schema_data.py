# ./compile_schema_data.py
import os
import sqlite3
from datetime import datetime

def export_database_schema(root_dir):
    """
    Exports all schema structures from the SQLite database at ./rpg_data.db into a single text file
    named compiled_schemas.txt in the ./SchemaManager/exports/ directory.

    Args:
        root_dir (str): The root directory of the project where the database resides.
    """
    # Define paths for the database and output file
    db_path = os.path.join(root_dir, "rpg_data.db")
    export_dir = os.path.join(root_dir, "SchemaManager", "exports")
    output_path = os.path.join(export_dir, "compiled_schemas.txt")
    
    # Create the export directory if it doesn't exist
    os.makedirs(export_dir, exist_ok=True)
    
    # Attempt to connect to the SQLite database
    try:
        with sqlite3.connect(db_path) as conn:
            cursor = conn.cursor()
            # Query to retrieve all schema definitions, ordered by type and name
            query = "SELECT type, name, sql FROM sqlite_master WHERE sql IS NOT NULL ORDER BY type, name;"
            cursor.execute(query)
            schema_data = cursor.fetchall()
    except sqlite3.OperationalError as e:
        print(f"Error connecting to database: {e}")
        return
    
    # Write schema information to the output file
    with open(output_path, 'w', encoding='utf-8') as outfile:
        # Write header with database name and export timestamp
        outfile.write("Database Schema for rpg_data.db\n")
        outfile.write(f"Exported on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        # Write each schema object's details
        for row in schema_data:
            type_, name, sql = row
            outfile.write(f"Type: {type_}\n")
            outfile.write(f"Name: {name}\n")
            outfile.write("SQL:\n")
            outfile.write(f"{sql}\n")
            outfile.write("\n----------\n\n")
    
    # Print confirmation message with relative path
    print(f"Database schema exported to {os.path.relpath(output_path, root_dir)}")

if __name__ == "__main__":
    # Set the root directory to the current working directory (project root)
    root_directory = os.getcwd()
    
    # Export the database schema
    export_database_schema(root_directory)
    
    # Final confirmation message
    print("Database schema export complete. Check compiled_schemas.txt in SchemaManager/exports/.")