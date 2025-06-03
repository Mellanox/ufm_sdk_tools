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

# Logger name constants
INITIAL_LOGGER_NAME = "ufm_plugin_conf_merger_initial"
DEFAULT_LOGGER_NAME = "ufm-plugin-configurations-merger"

# Initialize a module-level logger. Its name helps identify if it's been configured.
logger = logging.getLogger(INITIAL_LOGGER_NAME)
logger.setLevel(logging.INFO)

def setup_logger(plugin_name=None, log_level_input=None):
    """Configures the module-level logger."""
    global logger

    log_format = "[%(levelname)s] %(name)s: %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO) 

    final_logger_name = DEFAULT_LOGGER_NAME
    if plugin_name:
        final_logger_name = f'ufm-plugin-{plugin_name}-configurations-merger'

    _configured_logger = logging.getLogger(final_logger_name)

    actual_log_level = logging.INFO 
    if log_level_input:
        level_name_str = str(log_level_input).upper()
        actual_log_level = getattr(logging, level_name_str, logging.INFO)
    _configured_logger.setLevel(actual_log_level)

    logger = _configured_logger
    return logger

def merge_ini_files(old_file_path, new_file_path, merged_file_path):
    """
    Merges two INI configuration files while preserving structure and comments.

    This function takes an existing INI file (`old_file_path`) and a new INI file (`new_file_path`), 
    and generates a merged INI file (`merged_file_path`). The merging process ensures:
      - Section headers are preserved.
      - Key-value pairs from `new_file_path` are checked against `old_file_path`. 
        If a key exists in both, the value from `old_file_path` is retained.
      - Non-key lines, including empty lines and comments, are preserved from `new_file_path`.

    Args:
        old_file_path (str): Path to the original INI file.
        new_file_path (str): Path to the new INI file.
        merged_file_path (str): Path where the merged INI file will be saved.

    Returns:
        bool: True if merging is successful, False if any file is missing or an error occurs.
    """

    # If this function is called and the logger is still the initial placeholder,
    # it means __main__ block didn't run (e.g. imported as a module).
    # So, initialize the logger with default settings.
    if logger.name == INITIAL_LOGGER_NAME:
        setup_logger() # Initialize with default name and level

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
        logger.error("Error: Failed to read file %s", old_file_path)
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
        logger.exception("Failed to process a line or to write to merge file: %s", old_file)
        return False

    return True

if __name__ == "__main__":
    # Get file paths from command line arguments
    if len(sys.argv) < 3 or len(sys.argv) > 5:
        # Since setup_logger is called right after, print is fine for this usage message.
        print("Usage: python merge_configuration_files.py <old_file_path> "
              "<new_file_path> [plugin_name] [log_level]")
        sys.exit(1)

    old_file = sys.argv[1]
    new_file = sys.argv[2]

    cli_plugin_name = None
    if len(sys.argv) >= 4:
        cli_plugin_name = sys.argv[3]

    cli_log_level = None
    if len(sys.argv) == 5:
        cli_log_level = sys.argv[4] 

    # Configure the global logger instance using command-line arguments.
    setup_logger(plugin_name=cli_plugin_name, log_level_input=cli_log_level)

    tmp_merged_file = "temp_merged.cfg"

    result = merge_ini_files(old_file, new_file, tmp_merged_file)

    if not result:
        logger.error("Configuration file %s upgrade failed.", old_file)
        exit(1)

    else:
        try:
            logger.info("Move upgraded file %s to initial location %s", tmp_merged_file, old_file)
            
            # Move old file to backup
            backup_path = f"{old_file}.backup"
            shutil.move(old_file, backup_path)
            
            try:
                # Move new file to old file location
                shutil.move(tmp_merged_file, old_file)
                logger.info("Configuration file %s upgraded successfully.", old_file)
            except Exception:
                logger.exception("Failed to move upgraded file from %s to %s. Reverting changes.", tmp_merged_file, old_file)
                exit(1)
                
                # Attempt to restore the original file from backup
                shutil.move(backup_path, old_file)
                logger.info("Reverted to original configuration file %s.", old_file)
                exit(1)

        except Exception:
            logger.exception("Failed to move old configuration file to backup, from %s to %s.", old_file, backup_path)
            exit(1)
            