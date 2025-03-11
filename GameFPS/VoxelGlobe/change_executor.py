#!/usr/bin/env python3
import os
import sys
import re
import json
import argparse
from enum import Enum
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Optional, Union, Tuple
import difflib

class ChangeType(Enum):
    ADD = "add"      # Add lines to a file
    REPLACE = "replace"  # Replace lines in a file
    DELETE = "delete"    # Delete lines from a file
    CREATE = "create"    # Create a new file
    RENAME = "rename"    # Rename a file
    MOVE = "move"        # Move code from one file to another
    EXTRACT = "extract"  # Extract code to a new file

@dataclass
class Change:
    type: ChangeType
    file: str
    content: Optional[str] = None
    start_line: Optional[int] = None
    end_line: Optional[int] = None
    new_file: Optional[str] = None
    match_pattern: Optional[str] = None
    description: str = ""
    
    def to_dict(self):
        return {k: v.value if isinstance(v, Enum) else v for k, v in asdict(self).items() if v is not None}

@dataclass
class ChangeResult:
    success: bool
    message: str
    file: str
    diff: Optional[str] = None
    
    def to_dict(self):
        return {k: v for k, v in asdict(self).items() if v is not None}

@dataclass
class ExecutionLog:
    changes: List[Dict] = field(default_factory=list)
    results: List[Dict] = field(default_factory=list)
    summary: Dict = field(default_factory=dict)
    
    def to_dict(self):
        return asdict(self)
    
    def to_json(self):
        return json.dumps(self.to_dict(), indent=2)

class ChangeExecutor:
    def __init__(self, base_dir='.'):
        self.base_dir = base_dir
        self.log = ExecutionLog()
        
    def _read_file(self, file_path):
        full_path = os.path.join(self.base_dir, file_path)
        try:
            with open(full_path, 'r') as f:
                return f.read()
        except FileNotFoundError:
            return None
    
    def _write_file(self, file_path, content):
        full_path = os.path.join(self.base_dir, file_path)
        os.makedirs(os.path.dirname(full_path), exist_ok=True)
        with open(full_path, 'w') as f:
            f.write(content)
    
    def _find_pattern_position(self, content, pattern):
        """Find the start and end line numbers of a pattern in content."""
        if not pattern:
            return None, None
        
        lines = content.splitlines()
        pattern_lines = pattern.splitlines()
        
        if not pattern_lines:
            return None, None
        
        pattern_length = len(pattern_lines)
        
        for i in range(len(lines) - pattern_length + 1):
            matched = True
            for j in range(pattern_length):
                if lines[i + j] != pattern_lines[j]:
                    matched = False
                    break
            
            if matched:
                return i + 1, i + pattern_length  # 1-indexed line numbers
        
        return None, None
    
    def _get_diff(self, old_content, new_content, file_path):
        """Generate a unified diff between old and new content."""
        old_lines = old_content.splitlines(keepends=True) if old_content else []
        new_lines = new_content.splitlines(keepends=True) if new_content else []
        
        diff = difflib.unified_diff(
            old_lines, new_lines,
            fromfile=f'a/{file_path}',
            tofile=f'b/{file_path}',
            n=3  # Context lines
        )
        
        return ''.join(diff)
    
    def execute_change(self, change: Change) -> ChangeResult:
        """Execute a single change and return the result."""
        self.log.changes.append(change.to_dict())
        
        try:
            if change.type == ChangeType.CREATE:
                return self._create_file(change)
            elif change.type == ChangeType.ADD:
                return self._add_to_file(change)
            elif change.type == ChangeType.REPLACE:
                return self._replace_in_file(change)
            elif change.type == ChangeType.DELETE:
                return self._delete_from_file(change)
            elif change.type == ChangeType.RENAME:
                return self._rename_file(change)
            elif change.type == ChangeType.MOVE:
                return self._move_code(change)
            elif change.type == ChangeType.EXTRACT:
                return self._extract_code(change)
            else:
                return ChangeResult(
                    success=False,
                    message=f"Unknown change type: {change.type}",
                    file=change.file
                )
        except Exception as e:
            return ChangeResult(
                success=False,
                message=f"Error executing change: {str(e)}",
                file=change.file
            )
    
    def _create_file(self, change: Change) -> ChangeResult:
        """Create a new file with the given content."""
        if not change.content:
            return ChangeResult(
                success=False,
                message="No content provided for file creation",
                file=change.file
            )
        
        existing_content = self._read_file(change.file)
        if existing_content is not None:
            return ChangeResult(
                success=False,
                message=f"File already exists: {change.file}",
                file=change.file
            )
        
        self._write_file(change.file, change.content)
        diff = self._get_diff("", change.content, change.file)
        
        return ChangeResult(
            success=True,
            message=f"Created file: {change.file}",
            file=change.file,
            diff=diff
        )
    
    def _add_to_file(self, change: Change) -> ChangeResult:
        """Add content to a file at the specified location."""
        if not change.content:
            return ChangeResult(
                success=False,
                message="No content provided for addition",
                file=change.file
            )
        
        content = self._read_file(change.file)
        if content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        lines = content.splitlines()
        
        # If a pattern is provided, find its position
        if change.match_pattern:
            start, end = self._find_pattern_position(content, change.match_pattern)
            if start is None:
                return ChangeResult(
                    success=False,
                    message=f"Pattern not found in {change.file}",
                    file=change.file
                )
            change.start_line = start if change.start_line is None else start + change.start_line - 1
            change.end_line = end if change.end_line is None else start + change.end_line - 1
        
        # Determine insertion point
        if change.start_line is None:
            # If no line is specified, append to the end
            insertion_point = len(lines)
        else:
            # Adjust for 1-indexed line numbers
            insertion_point = change.start_line - 1
        
        # Insert the new content
        new_lines = lines[:insertion_point] + change.content.splitlines() + lines[insertion_point:]
        new_content = '\n'.join(new_lines)
        
        # Calculate diff
        diff = self._get_diff(content, new_content, change.file)
        
        # Write the updated content
        self._write_file(change.file, new_content)
        
        return ChangeResult(
            success=True,
            message=f"Added content at line {insertion_point + 1} in {change.file}",
            file=change.file,
            diff=diff
        )
    
    def _replace_in_file(self, change: Change) -> ChangeResult:
        """Replace content in a file between start_line and end_line."""
        if not change.content and not change.match_pattern:
            return ChangeResult(
                success=False,
                message="No content or pattern provided for replacement",
                file=change.file
            )
        
        content = self._read_file(change.file)
        if content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        lines = content.splitlines()
        
        # If a pattern is provided, find its position
        if change.match_pattern:
            start, end = self._find_pattern_position(content, change.match_pattern)
            if start is None:
                return ChangeResult(
                    success=False,
                    message=f"Pattern not found in {change.file}",
                    file=change.file
                )
            change.start_line = start if change.start_line is None else start
            change.end_line = end if change.end_line is None else end
        
        # Ensure start and end lines are valid
        if change.start_line is None or change.end_line is None:
            return ChangeResult(
                success=False,
                message="Start and end lines must be specified for replacement",
                file=change.file
            )
        
        # Adjust for 1-indexed line numbers
        start_idx = change.start_line - 1
        end_idx = change.end_line - 1
        
        # Replace the content
        new_content_lines = change.content.splitlines() if change.content else []
        new_lines = lines[:start_idx] + new_content_lines + lines[end_idx + 1:]
        new_content = '\n'.join(new_lines)
        
        # Calculate diff
        diff = self._get_diff(content, new_content, change.file)
        
        # Write the updated content
        self._write_file(change.file, new_content)
        
        return ChangeResult(
            success=True,
            message=f"Replaced content from line {start_idx + 1} to {end_idx + 1} in {change.file}",
            file=change.file,
            diff=diff
        )
    
    def _delete_from_file(self, change: Change) -> ChangeResult:
        """Delete content from a file between start_line and end_line."""
        content = self._read_file(change.file)
        if content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        lines = content.splitlines()
        
        # If a pattern is provided, find its position
        if change.match_pattern:
            start, end = self._find_pattern_position(content, change.match_pattern)
            if start is None:
                return ChangeResult(
                    success=False,
                    message=f"Pattern not found in {change.file}",
                    file=change.file
                )
            change.start_line = start if change.start_line is None else start
            change.end_line = end if change.end_line is None else end
        
        # Ensure start and end lines are valid
        if change.start_line is None or change.end_line is None:
            return ChangeResult(
                success=False,
                message="Start and end lines must be specified for deletion",
                file=change.file
            )
        
        # Adjust for 1-indexed line numbers
        start_idx = change.start_line - 1
        end_idx = change.end_line - 1
        
        # Delete the content
        new_lines = lines[:start_idx] + lines[end_idx + 1:]
        new_content = '\n'.join(new_lines)
        
        # Calculate diff
        diff = self._get_diff(content, new_content, change.file)
        
        # Write the updated content
        self._write_file(change.file, new_content)
        
        return ChangeResult(
            success=True,
            message=f"Deleted content from line {start_idx + 1} to {end_idx + 1} in {change.file}",
            file=change.file,
            diff=diff
        )
    
    def _rename_file(self, change: Change) -> ChangeResult:
        """Rename a file."""
        if not change.new_file:
            return ChangeResult(
                success=False,
                message="No new file name provided for rename operation",
                file=change.file
            )
        
        content = self._read_file(change.file)
        if content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        # Write to the new file
        self._write_file(change.new_file, content)
        
        # Remove the old file
        os.remove(os.path.join(self.base_dir, change.file))
        
        return ChangeResult(
            success=True,
            message=f"Renamed {change.file} to {change.new_file}",
            file=change.new_file
        )
    
    def _move_code(self, change: Change) -> ChangeResult:
        """Move code from one file to another."""
        if not change.new_file:
            return ChangeResult(
                success=False,
                message="No target file provided for move operation",
                file=change.file
            )
        
        # First, extract the code
        extract_result = self._extract_code(change)
        if not extract_result.success:
            return extract_result
        
        # Get the content to move
        source_content = self._read_file(change.file)
        if source_content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        lines = source_content.splitlines()
        
        # If a pattern is provided, find its position
        if change.match_pattern:
            start, end = self._find_pattern_position(source_content, change.match_pattern)
            if start is None:
                return ChangeResult(
                    success=False,
                    message=f"Pattern not found in {change.file}",
                    file=change.file
                )
            change.start_line = start if change.start_line is None else start
            change.end_line = end if change.end_line is None else end
        
        # Ensure start and end lines are valid
        if change.start_line is None or change.end_line is None:
            return ChangeResult(
                success=False,
                message="Start and end lines must be specified for move operation",
                file=change.file
            )
        
        # Adjust for 1-indexed line numbers
        start_idx = change.start_line - 1
        end_idx = change.end_line - 1
        
        # Extract the content to move
        move_content = '\n'.join(lines[start_idx:end_idx + 1])
        
        # Now add it to the target file
        add_change = Change(
            type=ChangeType.ADD,
            file=change.new_file,
            content=move_content,
            description=f"Adding code moved from {change.file}"
        )
        
        add_result = self.execute_change(add_change)
        if not add_result.success:
            return add_result
        
        # Delete from the source file
        delete_change = Change(
            type=ChangeType.DELETE,
            file=change.file,
            start_line=change.start_line,
            end_line=change.end_line,
            description=f"Removing code moved to {change.new_file}"
        )
        
        delete_result = self.execute_change(delete_change)
        if not delete_result.success:
            return delete_result
        
        return ChangeResult(
            success=True,
            message=f"Moved code from {change.file} to {change.new_file}",
            file=change.new_file,
            diff=add_result.diff + "\n" + delete_result.diff
        )
    
    def _extract_code(self, change: Change) -> ChangeResult:
        """Extract code from a file to a new file."""
        if not change.new_file:
            return ChangeResult(
                success=False,
                message="No new file name provided for extract operation",
                file=change.file
            )
        
        content = self._read_file(change.file)
        if content is None:
            return ChangeResult(
                success=False,
                message=f"File not found: {change.file}",
                file=change.file
            )
        
        lines = content.splitlines()
        
        # If a pattern is provided, find its position
        if change.match_pattern:
            start, end = self._find_pattern_position(content, change.match_pattern)
            if start is None:
                return ChangeResult(
                    success=False,
                    message=f"Pattern not found in {change.file}",
                    file=change.file
                )
            change.start_line = start if change.start_line is None else start
            change.end_line = end if change.end_line is None else end
        
        # Ensure start and end lines are valid
        if change.start_line is None or change.end_line is None:
            return ChangeResult(
                success=False,
                message="Start and end lines must be specified for extraction",
                file=change.file
            )
        
        # Adjust for 1-indexed line numbers
        start_idx = change.start_line - 1
        end_idx = change.end_line - 1
        
        # Extract the content
        extract_content = '\n'.join(lines[start_idx:end_idx + 1])
        
        # Create the new file
        create_change = Change(
            type=ChangeType.CREATE,
            file=change.new_file,
            content=extract_content,
            description=f"Creating new file with code extracted from {change.file}"
        )
        
        create_result = self.execute_change(create_change)
        
        return ChangeResult(
            success=create_result.success,
            message=f"Extracted code from {change.file} to {change.new_file}",
            file=change.new_file,
            diff=create_result.diff
        )
    
    def execute_changes(self, changes: List[Change]):
        """Execute multiple changes in sequence."""
        for change in changes:
            result = self.execute_change(change)
            self.log.results.append(result.to_dict())
            
            if not result.success:
                print(f"Error: {result.message}")
                break
        
        # Update summary
        success_count = sum(1 for r in self.log.results if r.get('success', False))
        failure_count = len(self.log.results) - success_count
        
        self.log.summary = {
            'total_changes': len(self.log.changes),
            'successful_changes': success_count,
            'failed_changes': failure_count,
            'completed': success_count == len(self.log.changes)
        }
    
    def get_log(self):
        """Get the execution log."""
        return self.log

def parse_args():
    parser = argparse.ArgumentParser(description='Execute code changes')
    parser.add_argument('--input', '-i', type=str, help='Input JSON file with changes')
    parser.add_argument('--output', '-o', type=str, help='Output JSON file for logs')
    parser.add_argument('--base-dir', '-d', type=str, default='.', help='Base directory for files')
    
    return parser.parse_args()

def main():
    args = parse_args()
    
    # Load changes from JSON file
    if args.input:
        with open(args.input, 'r') as f:
            change_data = json.load(f)
            
        changes = []
        for data in change_data:
            changes.append(Change(
                type=ChangeType(data['type']),
                file=data['file'],
                content=data.get('content'),
                start_line=data.get('start_line'),
                end_line=data.get('end_line'),
                new_file=data.get('new_file'),
                match_pattern=data.get('match_pattern'),
                description=data.get('description', '')
            ))
    else:
        # Example change for testing
        changes = [
            Change(
                type=ChangeType.CREATE,
                file='test.txt',
                content='This is a test file.\nIt has multiple lines.',
                description='Creating a test file'
            )
        ]
    
    # Execute changes
    executor = ChangeExecutor(base_dir=args.base_dir)
    executor.execute_changes(changes)
    
    # Get logs
    log = executor.get_log()
    
    # Output logs
    if args.output:
        with open(args.output, 'w') as f:
            f.write(log.to_json())
    else:
        print(log.to_json())

if __name__ == '__main__':
    main()