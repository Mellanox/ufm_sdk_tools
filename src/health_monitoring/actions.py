#
# Copyright © 2013-2025 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of Nvidia Corporation and its affiliates
# (the "Company") and all right, title, and interest in and to the software
# product, including all associated intellectual property rights, are and
# shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#
"""
Defines the default corrective actions for the Health Monitoring SDK.

This module contains functions that can be triggered when a health check fails,
such as restarting a supervised process.
"""

import subprocess
from typing import Dict, Any
import logging
import time

from . import constants as C


def restart_supervisor_program_action(config: Dict[str, Any], logger: logging.LoggerAdapter) -> None:
    """Enhanced restart with retry logic and verification."""
    program_name = config.get(C.ACTION_SUPERVISOR_PROGRAM_NAME)
    if not program_name:
        logger.error(
            "Restart action: '%s' not provided.", C.ACTION_SUPERVISOR_PROGRAM_NAME)
        return

    max_retries = 3
    retry_delay = 2

    for attempt in range(max_retries):
        try:
            logger.info(
                "Attempting to restart supervisord program: %s... (attempt %s/%s)",
                program_name, attempt + 1, max_retries)

            # First try to restart the program
            result = subprocess.run(
                ["supervisorctl", "restart", program_name],
                check=True, capture_output=True, text=True
            )

            logger.info(
                "Restart command for '%s' executed successfully. Output: %s",
                program_name, result.stdout.strip()
            )

            # Wait a moment for the process to start
            time.sleep(2)

            # Verify the restart was successful by checking status
            status_result = subprocess.run(
                ["supervisorctl", "status", program_name],
                capture_output=True, text=True, check=False
            )

            if status_result.returncode == 0 and "RUNNING" in status_result.stdout:
                logger.info(
                    "Supervisord program '%s' restarted successfully and is running.",
                    program_name
                )
                return
            else:
                logger.warning(
                    "Program '%s' restart command succeeded but process not running. Status: %s",
                    program_name, status_result.stdout.strip()
                )

        except subprocess.CalledProcessError as ex:
            logger.error(
                "Failed to restart supervisord program '%s' (attempt %s/%s). RC: %s\nStdout: %s\nStderr: %s",
                program_name, attempt + 1, max_retries, ex.returncode, 
                ex.stdout.strip(), ex.stderr.strip()
            )
        except FileNotFoundError:
            logger.error(
                "supervisorctl command not found. Make sure Supervisor is installed and in PATH.")
            return
        except Exception:  # pylint: disable=broad-except
            logger.exception(
                "An unexpected error occurred while trying to restart '%s' (attempt %s/%s).", 
                program_name, attempt + 1, max_retries
            )

        if attempt < max_retries - 1:
            logger.info("Retrying restart in %s seconds...", retry_delay)
            time.sleep(retry_delay)

    logger.error(
        "Failed to restart supervisord program '%s' after %s attempts.",
        program_name, max_retries
    )
