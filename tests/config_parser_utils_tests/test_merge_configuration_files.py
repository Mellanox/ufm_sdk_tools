"""
@copyright:
    Copyright (C) Mellanox Technologies Ltd. 2014-2025.  ALL RIGHTS RESERVED.

    This software product is a proprietary product of Mellanox Technologies
    Ltd. (the "Company") and all right, title, and interest in and to the
    software product, including all associated intellectual property rights,
    are and shall remain exclusively with the Company.

    This software product is governed by the End User License Agreement
    provided with the software product.

@author: Miryam Schwartz
@date:   Feb 12, 2025
"""

import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../src")))
from src.config_parser_utils.merge_configuration_files import merge_ini_files

# Get the directory where this test file is located
TEST_DIR = os.path.dirname(os.path.abspath(__file__))

# Define file paths relative to the test directory
OLD_FILE = os.path.join(TEST_DIR, "old.cfg")
NEW_FILE = os.path.join(TEST_DIR, "new.cfg")
EXPECTED_FILE = os.path.join(TEST_DIR, "expected.cfg")
OUTPUT_FILE = os.path.join(TEST_DIR, "output.cfg")

def files_are_equal_ignore_trailing_spaces(file1, file2):
    with open(file1, 'r', encoding='utf-8') as f1, open(file2, 'r', encoding='utf-8') as f2:
        lines1 = [line.rstrip() for line in f1]  # Remove trailing spaces
        lines2 = [line.rstrip() for line in f2]  # Remove trailing spaces
    return lines1 == lines2  # Compare line by line

def test_merge_ini_files():
    """Tests if merge_ini_files correctly merges INI files while preserving formatting and comments."""

    # Run the merge function
    merge_ini_files(OLD_FILE, NEW_FILE, OUTPUT_FILE)

    # Compare output with expected file
    assert files_are_equal_ignore_trailing_spaces(OUTPUT_FILE, EXPECTED_FILE), "output cfg file does not match expected output (ignoring trailing spaces)."

if __name__ == "__main__":
    test_merge_ini_files()
