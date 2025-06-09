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
Defines constants for the Health Monitoring SDK.

This module centralizes all hardcoded strings and magic numbers used throughout
the SDK, such as configuration keys, default values, and internal state keys.
This makes the SDK more maintainable and easier to configure.
"""

# --- Task Configuration Keys ---
TASK_NAME = "name"
TASK_ENABLED = "enabled"
TASK_PROCESS_NAME = "process_name"
TASK_CHECK_INTERVAL = "check_interval_seconds"
TASK_CHECK_TYPE = "check_type"
TASK_CHECK_CONFIG = "check_config"
TASK_ACTION_ON_FAILURE = "action_on_failure"
TASK_ACTION_CONFIG = "action_config"
TASK_CONSECUTIVE_FAILURES_THRESHOLD = "consecutive_failures_threshold"
TASK_RECOVER_AFTER_SUCCESSES_THRESHOLD = "recover_after_successes_threshold"

# --- Check & Action Types ---
CHECK_TYPE_HTTP_GET = "http_get"
CHECK_TYPE_COMMAND = "command"
ACTION_TYPE_RESTART_SUPERVISOR_PROGRAM = "restart_supervisor_program"

# --- HTTP Get Check Config Keys ---
HTTP_URL = "url"
HTTP_TIMEOUT = "timeout"
HTTP_EXPECTED_STATUS_CODE = "expected_status_code"

# --- Command Check Config Keys ---
COMMAND_CMD = "command"
COMMAND_EXPECTED_RETURN_CODE = "expected_return_code"

# --- Restart Action Config Keys ---
ACTION_SUPERVISOR_PROGRAM_NAME = "supervisor_program_name"

# --- Default Values ---
DEFAULT_HTTP_TIMEOUT = 5
DEFAULT_HTTP_STATUS_CODE_OK = 200
DEFAULT_COMMAND_RETURN_CODE_OK = 0
DEFAULT_CHECK_INTERVAL = 30
DEFAULT_FAILURE_THRESHOLD = 1
DEFAULT_LOOP_SLEEP_INTERVAL = 1.0
DEFAULT_LOGGER_NAME = "HealthMonitor"
DEFAULT_TASK_NAME = "UnknownTask"

# --- Internal State Keys ---
STATE_LAST_CHECK_TIME = "last_check_time"
STATE_CONSECUTIVE_FAILURES = "consecutive_failures"
STATE_IS_UNHEALTHY = "is_unhealthy"
