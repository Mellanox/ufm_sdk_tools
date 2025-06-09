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

from . import constants as C


def restart_supervisor_program_action(config: Dict[str, Any], logger: logging.LoggerAdapter) -> None:
    """Restarts a program using supervisorctl."""
    program_name = config.get(C.ACTION_SUPERVISOR_PROGRAM_NAME)
    if not program_name:
        logger.error(
            "Restart action: '%s' not provided.", C.ACTION_SUPERVISOR_PROGRAM_NAME)
        return

    try:
        logger.info(
            "Attempting to restart supervisord program: %s...", program_name)
        subprocess.run(
            ["supervisorctl", "restart", program_name],
            check=True, capture_output=True, text=True
        )
        logger.info(
            "Supervisord program '%s' restart command issued successfully.",
            program_name
        )
    except subprocess.CalledProcessError as ex:
        logger.error(
            "Failed to restart supervisord program '%s'. RC: %s\nStdout: %s\nStderr: %s",
            program_name, ex.returncode, ex.stdout.strip(), ex.stderr.strip()
        )
    except FileNotFoundError:
        logger.error(
            "supervisorctl command not found. Make sure Supervisor is installed and in PATH.")
    except Exception:  # pylint: disable=broad-except
        logger.exception(
            "An unexpected error occurred while trying to restart '%s'.", program_name)
