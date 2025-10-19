#!/usr/bin/env python3
"""
update_license_headers.py - Update all files to Apache 2.0 license
Usage: python3 scripts/update_license_headers.py
"""

import os
import re
from pathlib import Path

# Apache 2.0 header template
APACHE_HEADER = """/**
 * @file {filename}
 * @brief {brief}
 *
 * Copyright (c) 2025 Ojima Abraham
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Ojima Abraham
 * @date 2025
 */
"""


def extract_brief(content):
    """Extract @brief from existing header if present"""
    match = re.search(r"@brief\s+(.+?)(?:\n\s*\*\s*\n|\n\s*\*/)", content, re.DOTALL)
    if match:
        brief = match.group(1).strip()
        # Clean up multi-line briefs
        brief = " ".join(brief.split())
        return brief
    return "Description needed"


def has_old_license(content):
    """Check if file has old BSL license"""
    return "Business Source License" in content or "BSL" in content


def remove_old_header(content):
    """Remove old license header"""
    # Remove everything from /** to first non-comment line
    pattern = r"^/\*\*.*?\*/\s*\n"
    match = re.search(pattern, content, re.DOTALL | re.MULTILINE)
    if match:
        return content[match.end() :]
    return content


def update_file(filepath):
    """Update a single file with Apache 2.0 license"""
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # Extract brief if exists
    brief = extract_brief(content)

    # Remove old header
    clean_content = remove_old_header(content)

    # Create new header
    new_header = APACHE_HEADER.format(filename=filepath.name, brief=brief)

    # Combine
    new_content = new_header + "\n" + clean_content

    # Write back
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"✓ Updated {filepath}")


def main():
    project_root = Path(__file__).parent.parent

    # File extensions to process
    extensions = [".cpp", ".h", ".hpp"]

    # Directories to process
    dirs = ["src", "include", "apps", "tests", "examples", "benchmarks"]

    count = 0
    for dir_name in dirs:
        dir_path = project_root / dir_name
        if not dir_path.exists():
            continue

        for ext in extensions:
            for filepath in dir_path.rglob(f"*{ext}"):
                update_file(filepath)
                count += 1

    print(f"\n✓ Updated {count} files to Apache 2.0 license")
    print("\nNext steps:")
    print("  1. Review changes: git diff")
    print("  2. Update LICENSE file")
    print("  3. Commit: git commit -am 'Update to Apache 2.0 license'")


if __name__ == "__main__":
    main()
