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


from .core import HealthMonitor, TaskLoggerAdapter, get_task_logger
from .checks import http_get_check, command_check
from .actions import restart_supervisor_program_action
from . import constants as C

# Pre-populated default check and action functions for convenience.
# The HealthMonitor class can register more or override these.

DEFAULT_CHECK_FUNCTIONS = {
    C.CHECK_TYPE_HTTP_GET: http_get_check,
    C.CHECK_TYPE_COMMAND: command_check,
}

DEFAULT_ACTION_FUNCTIONS = {
    C.ACTION_TYPE_RESTART_SUPERVISOR_PROGRAM: restart_supervisor_program_action,
}

__all__ = [
    "HealthMonitor",
    "TaskLoggerAdapter",
    "get_task_logger",
    "http_get_check",
    "command_check",
    "restart_supervisor_program_action",
    "DEFAULT_CHECK_FUNCTIONS",
    "DEFAULT_ACTION_FUNCTIONS",
    "C" # Export constants as 'C' for plugin use
]
