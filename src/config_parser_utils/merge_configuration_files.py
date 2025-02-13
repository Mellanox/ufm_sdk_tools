"""
This script merges two INI configuration files, preserving existing values from an old file 
while integrating updates from a new file. It ensures that missing or modified settings 
are properly retained and logs any errors encountered during the process.

@copyright:
    Copyright (C) Mellanox Technologies Ltd. 2014-2025.  ALL RIGHTS RESERVED.

    This software product is a proprietary product of Mellanox Technologies
    Ltd. (the "Company") and all right, title, and interest in and to the
    software product, including all associated intellectual property rights,
    are and shall remain exclusively with the Company.

    This software product is governed by the End User License Agreement
    provided with the software product.

@author: Miryam Schwartz
@date:   Nov 14, 2024
"""
import configparser
import logging
from logging.handlers import SysLogHandler
import shutil
import sys
import os
import re

CFG_LINE_RGX = r"^(\S+)\s*=\s*(.*)$"

def setup_logger():
    """Configures and returns a logger that sends logs to syslog."""
    logger = logging.getLogger('upgrade')
    logger.setLevel(logging.DEBUG)

    # Create a syslog handler for the local syslog daemon
    syslog_handler = SysLogHandler(address=('localhost', 514))

    # Create a log format
    formatter = logging.Formatter(
        fmt="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%b %d %H:%M:%S"
    )
    syslog_handler.setFormatter(formatter)

    # Add the handler to the logger
    logger.addHandler(syslog_handler)

    return logger

def merge_ini_files(old_file_path, new_file_path, merged_file_path):
    # Check if files exist
    for file_path in (old_file_path, new_file_path):
        if not os.path.isfile(file_path):
            logger.error("file %s does not exist.", file_path)
            return False

    # Create a configparser object
    config_old = configparser.ConfigParser()
    config_old.optionxform = str  # Preserve case sensitivity

    res = config_old.read(old_file_path)
    if not res:
        logger.error(f"Error: Failed to read file {old_file_path}")
        return False

    try:
        with open(new_file_path, 'r', encoding='utf-8') as nf, open(merged_file_path, 'w', encoding='utf-8') as of:
            section = None
            for line in nf:
                stripped = line.strip()

                # Preserve section headers
                if stripped.startswith("[") and stripped.endswith("]"):
                    section = stripped[1:-1]
                    of.write(line)
                    continue

                # Match key-value pairs, preserving inline comments
                match = re.match(CFG_LINE_RGX, line)
                if match and section:
                    key, new_value = match.groups()
                    if config_old.has_section(section) and config_old.has_option(section, key):
                        new_value = (config_old.get(section, key)).strip()
                    # Write back the line with preserved comments
                    of.write(f"{key} = {new_value}\n")
                else:
                    of.write(line)  # Preserve non-key lines (empty lines, comments)
   
    except Exception as e:
        logger.error(f"Failed to process a line or to write to merge file: {e}")
        return False

    return True

if __name__ == "__main__":
    logger = setup_logger()

    # Get file paths from command line arguments
    if len(sys.argv) != 3:
        logger.error("Usage: python merge_configuration_files.py <old_file_path> <new_file_path>")
        sys.exit(1)

    old_file = sys.argv[1]
    new_file = sys.argv[2]

    tmp_merged_file = "temp_merged.cfg"

    result = merge_ini_files(old_file, new_file, tmp_merged_file)

    if not result:
        logger.error("Configuration file upgrade failed.")
        exit(1)

    else:

        try:
            logger.info("Configuration file upgraded successfully.")
            logger.info(f"Move upgraded file {tmp_merged_file} to initial location {new_file}")
            shutil.move(new_file, f"{old_file}.backup")
            shutil.move(tmp_merged_file, new_file)

        except Exception as e:
            logger.error(f"Failed to move upgraded file: {e}")
