#
# Copyright © 2013-2025 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of Nvidia Corporation and its affiliates
# (the "Company") and all right,title, and interest in and to the software
# product, including all associated intellectual property rights, are and
# shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#
"""
Core components for the Health Monitoring SDK.

This module contains the main HealthMonitor class that drives the monitoring
loop, as well as logging utilities for creating task-specific loggers.
"""

import time
import logging
from typing import List, Dict, Any, Callable

from . import constants as C

# --- Logging Setup ---
class TaskLoggerAdapter(logging.LoggerAdapter):
    # pylint: disable=too-few-public-methods
    """A logger adapter to prepend task names to log messages."""
    def process(self, msg, kwargs):
        """Prepends the task name to the log message."""
        task_name = self.extra.get(C.TASK_NAME, C.DEFAULT_TASK_NAME)
        return f'[{task_name}] {msg}', kwargs

def get_task_logger(base_logger_name: str, task_name: str) -> TaskLoggerAdapter:
    """Creates a logger adapter for a specific task."""
    logger = logging.getLogger(base_logger_name)
    return TaskLoggerAdapter(logger, {C.TASK_NAME: task_name})

# --- HealthMonitor Class ---
class HealthMonitor:
    # pylint: disable=too-few-public-methods,too-many-instance-attributes
    """Main class to run and manage the health monitoring loop."""
    def __init__(
        self,
        monitored_tasks: List[Dict[str, Any]],
        check_functions: Dict[str, Callable[[Dict[str, Any], TaskLoggerAdapter], bool]],
        action_functions: Dict[str, Callable[[Dict[str, Any], TaskLoggerAdapter], None]],
        base_logger_name: str = C.DEFAULT_LOGGER_NAME,
        loop_sleep_interval: float = C.DEFAULT_LOOP_SLEEP_INTERVAL
    ):
        """
        Initializes the HealthMonitor.

        Args:
            monitored_tasks: A list of task configurations.
            check_functions: A dictionary mapping check_type strings to
                             check functions.
            action_functions: A dictionary mapping action_on_failure strings
                              to action functions.
            base_logger_name: The name of the base logger to use for
                              task loggers.
            loop_sleep_interval: Time in seconds for the main loop to sleep.
        """
        self.monitored_tasks = monitored_tasks
        self.check_functions = check_functions
        self.action_functions = action_functions
        self.base_logger_name = base_logger_name
        self.loop_sleep_interval = loop_sleep_interval
        self.task_states: Dict[str, Dict[str, Any]] = {}
        self.root_logger = logging.getLogger(self.base_logger_name)

        self._initialize_task_states()

    def _initialize_task_states(self):
        """Initializes the runtime state for each configured task."""
        for task_config in self.monitored_tasks:
            task_name = task_config.get(C.TASK_NAME)
            if not task_name:
                self.root_logger.warning("A task is missing a '%s'. It will be skipped.",
                                         C.TASK_NAME)
                continue
            self.task_states[task_name] = {
                C.STATE_LAST_CHECK_TIME: 0,
                C.STATE_CONSECUTIVE_FAILURES: 0,
                C.STATE_IS_UNHEALTHY: False,
            }

    def _handle_healthy_task(self, state, logger):
        """Handles the logic for a task that has passed its health check."""
        if state[C.STATE_IS_UNHEALTHY]:
            logger.info("Service has recovered.")
            state[C.STATE_IS_UNHEALTHY] = False
        state[C.STATE_CONSECUTIVE_FAILURES] = 0

    def _trigger_failure_action(self, task_config, logger):
        # pylint: disable=no-self-use
        """Triggers the configured action for a failed task."""
        action_type = task_config.get(C.TASK_ACTION_ON_FAILURE)
        action_function = self.action_functions.get(action_type)
        if not action_function:
            logger.error(
                "Unknown action_on_failure: '%s'. No action taken.", action_type)
            return

        action_cfg = task_config.get(C.TASK_ACTION_CONFIG, {})
        # Auto-fill supervisor_program_name if not provided in action_config
        is_restart_action = action_type == C.ACTION_TYPE_RESTART_SUPERVISOR_PROGRAM
        is_name_missing = C.ACTION_SUPERVISOR_PROGRAM_NAME not in action_cfg
        process_name = task_config.get(C.TASK_PROCESS_NAME)

        if is_restart_action and is_name_missing and process_name:
            action_cfg[C.ACTION_SUPERVISOR_PROGRAM_NAME] = process_name

        action_function(action_cfg, logger)

    def _handle_unhealthy_task(self, task_config, state, logger):
        """Handles the logic for a task that has failed its health check."""
        state[C.STATE_CONSECUTIVE_FAILURES] += 1
        logger.warning("Health check failed. Consecutive failures: %s.",
                     state[C.STATE_CONSECUTIVE_FAILURES])

        failure_threshold = task_config.get(C.TASK_CONSECUTIVE_FAILURES_THRESHOLD,
                                          C.DEFAULT_FAILURE_THRESHOLD)

        is_unhealthy = state[C.STATE_IS_UNHEALTHY]
        threshold_reached = state[C.STATE_CONSECUTIVE_FAILURES] >= failure_threshold

        if threshold_reached and not is_unhealthy:
            action = task_config.get(C.TASK_ACTION_ON_FAILURE)
            logger.error(
                "Reached %s consecutive failures (threshold is %s). Triggering: %s.",
                state[C.STATE_CONSECUTIVE_FAILURES], failure_threshold, action
            )
            state[C.STATE_IS_UNHEALTHY] = True
            self._trigger_failure_action(task_config, logger)
        elif is_unhealthy:
            logger.warning("Service unhealthy. Waiting for recovery before new actions.")

    def _run_task_check(self, task_config):
        """Runs the health check for a single task."""
        task_name = task_config.get(C.TASK_NAME)
        if not task_name or task_name not in self.task_states:
            return  # Skip misconfigured or unnamed tasks

        if not task_config.get(C.TASK_ENABLED, False):
            return

        state = self.task_states[task_name]
        check_interval = task_config.get(C.TASK_CHECK_INTERVAL,
                                         C.DEFAULT_CHECK_INTERVAL)

        if time.time() - state[C.STATE_LAST_CHECK_TIME] >= check_interval:
            state[C.STATE_LAST_CHECK_TIME] = time.time()
            logger = get_task_logger(self.base_logger_name, task_name)
            logger.info("Performing health check...")

            check_type = task_config.get(C.TASK_CHECK_TYPE)
            check_function = self.check_functions.get(check_type)
            if not check_function:
                logger.error("Unknown check_type: '%s'. Skipping task.", check_type)
                return

            is_healthy = check_function(
                task_config.get(C.TASK_CHECK_CONFIG, {}), logger)

            if is_healthy:
                self._handle_healthy_task(state, logger)
            else:
                self._handle_unhealthy_task(task_config, state, logger)

    def run(self):
        """
        Runs the main health monitoring loop indefinitely.
        This method continuously iterates through the monitored tasks,
        executes their health checks based on their configured intervals,
        and triggers actions upon failure.
        """
        self.root_logger.info("Health Monitor started.")
        while True:
            for task_config in self.monitored_tasks:
                self._run_task_check(task_config)
            time.sleep(self.loop_sleep_interval)
