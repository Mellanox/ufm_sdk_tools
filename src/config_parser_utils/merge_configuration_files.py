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
@date:   Nov 14, 2024

This script merges two INI configuration files, preserving existing values from an old file 
while integrating updates from a new file. It ensures that missing or modified settings 
are properly retained and logs any errors encountered during the process.
"""
import configparser
import logging
from logging.handlers import SysLogHandler
import shutil
import sys
import os
import re

# Matches a config line "key = value".
# Captures the "key" (non-whitespace) and the "value" (any characters).
CFG_LINE_RGX = r"^(\S+)\s*=\s*(.*)$"

def setup_logger(plugin_name=None, log_level=None):
    """Configures and returns a logger that sends logs to syslog."""
    logger_name = f'ufm-plugin-{plugin_name}-configurations-merger' if plugin_name else 'ufm-plugin-configurations-merger'
    logger = logging.getLogger(logger_name)
    if log_level:
        logger.setLevel(getattr(logging, log_level, logging.INFO))
    else:
        logger.setLevel(logging.INFO)

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
        logger.error("Error: Failed to read file %s" % old_file_path)
        return False

    try:
        with open(new_file_path, 'r', encoding='utf-8') as new_file, open(merged_file_path, 'w', encoding='utf-8') as old_file:
            section = None
            for line in new_file:
                stripped = line.strip()

                # Preserve section headers
                if stripped.startswith("[") and stripped.endswith("]"):
                    section = stripped[1:-1]
                    old_file.write(line)
                    continue

                # Match key-value pairs, preserving inline comments
                match = re.match(CFG_LINE_RGX, line)
                if match and section:
                    key, new_value = match.groups()
                    if config_old.has_section(section) and config_old.has_option(section, key):
                        new_value = (config_old.get(section, key)).strip()
                    # Write back the line with preserved comments
                    old_file.write(f"{key} = {new_value.strip()}\n")
                else:
                    old_file.write(line)  # Preserve non-key lines (empty lines, comments)
   
    except Exception:
        logger.exception("Failed to process a line or to write to merge file: %s")
        return False

    return True

if __name__ == "__main__":
    # Get file paths from command line arguments
    if len(sys.argv) < 3 or len(sys.argv) > 5:
        print("Usage: python merge_configuration_files.py <old_file_path> <new_file_path> [plugin_name] [log_level]")
        sys.exit(1)

    old_file = sys.argv[1]
    new_file = sys.argv[2]

    # Check for optional arguments
    plugin_name = None
    if len(sys.argv) >= 4:
        plugin_name = sys.argv[3]

    log_level = None
    if len(sys.argv) == 5:
        log_level = sys.argv[4].upper()

    logger = setup_logger(plugin_name, log_level)

    tmp_merged_file = "temp_merged.cfg"

    result = merge_ini_files(old_file, new_file, tmp_merged_file)

    if not result:
        logger.error("Configuration file upgrade failed.")
        exit(1)

    else:
        try:
            logger.info("Move upgraded file %s to initial location %s" % (tmp_merged_file, old_file))
            shutil.move(old_file, f"{old_file}.backup")
            shutil.move(tmp_merged_file, old_file)
            logger.info("Configuration file upgraded successfully.")

        except Exception:
            logger.exception("Failed to move upgraded file from %s to %s." % (tmp_merged_file, old_file))
