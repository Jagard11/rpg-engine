# ./FrontEnd/CharacterInfo/api/classBrowserApi.py

import sqlite3
import streamlit as st
from typing import Dict, List, Optional, Tuple
import json
from dataclasses import dataclass
from datetime import datetime
from flask import request, jsonify

@dataclass
class ClassFilterParams:
    search: Optional[str] = None
    category_id: Optional[int] = None
    class_type: Optional[str] = None
    is_racial: Optional[bool] = None
    has_prerequisites: Optional[bool] = None
    page: int = 1
    page_size: int = 20
    sort_by: str = 'name'
    sort_direction: str = 'asc'

class ClassBrowserAPI:
    def __init__(self):
        self.db_path = 'rpg_data.db'

    def get_db_connection(self) -> sqlite3.Connection:
        """Create a database connection"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def get_classes(self, filters: ClassFilterParams) -> Tuple[List[Dict], int]:
        """Get filtered classes with pagination"""
        conn = self.get_db_connection()
        try:
            # Build query conditions
            conditions = []
            params = []

            if filters.search:
                conditions.append("(c.name LIKE ? OR c.description LIKE ?)")
                search_term = f"%{filters.search}%"
                params.extend([search_term, search_term])

            if filters.category_id:
                conditions.append("c.category_id = ?")
                params.append(filters.category_id)

            if filters.class_type:
                conditions.append("ct.name = ?")
                params.append(filters.class_type)

            if filters.is_racial is not None:
                conditions.append("c.is_racial = ?")
                params.append(filters.is_racial)

            if filters.has_prerequisites:
                conditions.append("""
                    EXISTS (
                        SELECT 1 FROM class_prerequisites cp 
                        WHERE cp.class_id = c.id
                    )
                """)

            # Construct WHERE clause
            where_clause = " AND ".join(conditions) if conditions else "1=1"

            # Count total results
            count_query = f"""
                SELECT COUNT(*) 
                FROM classes c
                JOIN class_types ct ON c.class_type = ct.id
                WHERE {where_clause}
            """
            cursor = conn.execute(count_query, params)
            total_count = cursor.fetchone()[0]

            # Get paginated results
            offset = (filters.page - 1) * filters.page_size
            main_query = f"""
                SELECT 
                    c.id,
                    c.name,
                    c.description,
                    ct.name as type,
                    cc.name as category,
                    cs.name as subcategory,
                    c.is_racial,
                    c.base_hp,
                    c.base_mp,
                    c.base_physical_attack,
                    c.base_physical_defense,
                    c.base_agility,
                    c.base_magical_attack,
                    c.base_magical_defense,
                    c.base_resistance,
                    c.base_special
                FROM classes c
                JOIN class_types ct ON c.class_type = ct.id
                JOIN class_categories cc ON c.category_id = cc.id
                JOIN class_subcategories cs ON c.subcategory_id = cs.id
                WHERE {where_clause}
                ORDER BY {filters.sort_by} {filters.sort_direction}
                LIMIT ? OFFSET ?
            """
            params.extend([filters.page_size, offset])
            
            cursor = conn.execute(main_query, params)
            classes = [dict(row) for row in cursor.fetchall()]

            return classes, total_count

        finally:
            conn.close()

    def get_class_details(self, class_id: int) -> Dict:
        """Get detailed information for a specific class"""
        conn = self.get_db_connection()
        try:
            # Get prerequisites
            prereq_query = """
                SELECT 
                    cp.prerequisite_type,
                    cp.target_id,
                    cp.required_level,
                    cp.min_value,
                    cp.max_value,
                    c2.name as target_name,
                    cc.name as category_name,
                    cs.name as subcategory_name
                FROM class_prerequisites cp
                LEFT JOIN classes c2 ON cp.target_id = c2.id
                LEFT JOIN class_categories cc ON cp.target_id = cc.id
                LEFT JOIN class_subcategories cs ON cp.target_id = cs.id
                WHERE cp.class_id = ?
                ORDER BY cp.prerequisite_group, cp.id
            """
            cursor = conn.execute(prereq_query, (class_id,))
            prerequisites = [dict(row) for row in cursor.fetchall()]

            # Get exclusions
            excl_query = """
                SELECT 
                    ce.exclusion_type,
                    ce.target_id,
                    ce.min_value,
                    ce.max_value,
                    c2.name as target_name,
                    cc.name as category_name,
                    cs.name as subcategory_name
                FROM class_exclusions ce
                LEFT JOIN classes c2 ON ce.target_id = c2.id
                LEFT JOIN class_categories cc ON ce.target_id = cc.id
                LEFT JOIN class_subcategories cs ON ce.target_id = cs.id
                WHERE ce.class_id = ?
            """
            cursor = conn.execute(excl_query, (class_id,))
            exclusions = [dict(row) for row in cursor.fetchall()]

            return {
                'prerequisites': prerequisites,
                'exclusions': exclusions
            }

        finally:
            conn.close()

    def get_categories(self) -> List[Dict]:
        """Get all class categories with their subcategories"""
        conn = self.get_db_connection()
        try:
            query = """
                SELECT 
                    cc.id,
                    cc.name,
                    cc.is_racial,
                    cs.id as subcategory_id,
                    cs.name as subcategory_name
                FROM class_categories cc
                LEFT JOIN class_subcategories cs ON 1=1
                ORDER BY cc.name, cs.name
            """
            cursor = conn.execute(query)
            rows = cursor.fetchall()

            # Organize into hierarchy
            categories = {}
            for row in rows:
                cat_id = row['id']
                if cat_id not in categories:
                    categories[cat_id] = {
                        'id': cat_id,
                        'name': row['name'],
                        'is_racial': row['is_racial'],
                        'subcategories': []
                    }
                if row['subcategory_id']:
                    categories[cat_id]['subcategories'].append({
                        'id': row['subcategory_id'],
                        'name': row['subcategory_name']
                    })

            return list(categories.values())

        finally:
            conn.close()

    def get_class_types(self) -> List[Dict]:
        """Get all class types"""
        conn = self.get_db_connection()
        try:
            query = "SELECT id, name FROM class_types ORDER BY id"
            cursor = conn.execute(query)
            return [dict(row) for row in cursor.fetchall()]
        finally:
            conn.close()

