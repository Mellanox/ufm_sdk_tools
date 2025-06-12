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
Defines the default health check functions for the Health Monitoring SDK.

This module contains functions that can be used to verify the health of a
process, such as checking an HTTP endpoint or running a command.
"""
import logging
import subprocess
from typing import Dict, Any

import requests

from . import constants as C


def http_get_check(config: Dict[str, Any], logger: logging.LoggerAdapter) -> bool:
    """Performs an HTTP GET health check."""
    url = config.get(C.HTTP_URL)
    timeout = config.get(C.HTTP_TIMEOUT, C.DEFAULT_HTTP_TIMEOUT)
    expected_status_code = config.get(
        C.HTTP_EXPECTED_STATUS_CODE, C.DEFAULT_HTTP_STATUS_CODE_OK)

    if not url:
        logger.error("HTTP check: '%s' not provided in config.", C.HTTP_URL)
        return False
    try:
        response = requests.get(url, timeout=timeout)
        if response.status_code == expected_status_code:
            logger.info("HTTP check successful for %s.", url)
            return True

        logger.warning(
            "HTTP check for %s failed. Status: %s (expected %s).",
            url, response.status_code, expected_status_code
        )
        return False
    except requests.exceptions.RequestException:
        logger.exception("HTTP check for %s failed.", url)
        return False

def command_check(config: Dict[str, Any], logger: logging.LoggerAdapter) -> bool:
    """Performs a command execution health check."""
    command = config.get(C.COMMAND_CMD)
    expected_return_code = config.get(
        C.COMMAND_EXPECTED_RETURN_CODE, C.DEFAULT_COMMAND_RETURN_CODE_OK)

    if not command or not isinstance(command, list):
        logger.error(
            "Command check: '%s' (list of strings) not provided or not a list.",
            C.COMMAND_CMD)
        return False
    try:
        process = subprocess.run(
            command, capture_output=True, text=True, check=False)
        if process.returncode == expected_return_code:
            logger.info("Command check successful: %s.", ' '.join(command))
            return True

        log_msg = (
            "Command check failed: %s. RC: %s (expected %s).\n"
            "--> STDOUT: %s\n"
            "--> STDERR: %s"
        )
        logger.warning(
            log_msg,
            ' '.join(command), process.returncode, expected_return_code,
            process.stdout.strip(), process.stderr.strip()
        )
        return False
    except FileNotFoundError:
        logger.error("Command check failed: Command not found - %s.", command[0])
        return False
    except Exception:  # pylint: disable=broad-except
        logger.exception("Command check for %s failed.", ' '.join(command))
        return False
