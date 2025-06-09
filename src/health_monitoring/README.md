# UFM Health Monitoring SDK

This SDK provides a generic, configurable framework for monitoring the health of supervised processes and taking corrective actions.

## Overview

The primary goal of this SDK is to allow UFM plugins (or other applications using `supervisord`) to easily implement custom health checking logic for their managed processes. Instead of relying solely on `supervisord`'s basic restart capabilities, this SDK enables more intelligent monitoring (e.g., API endpoint checks, command execution checks) and defined responses to failures (e.g., restarting a specific process after N consecutive failures).

## Architecture

The SDK is composed of a few key components:

1.  **`core.py`**: 
    *   `HealthMonitor`: The main class that drives the monitoring loop. It takes a configuration of tasks, check functions, and action functions.
    *   `TaskLoggerAdapter` & `get_task_logger`: Utilities for creating task-specific loggers, making it easier to trace activity for individual monitored processes.

2.  **`checks.py`**: 
    *   Contains predefined functions for common health check types (e.g., `http_get_check`, `command_check`).
    *   These functions are designed to be extensible; users can add their own custom check functions.

3.  **`actions.py`**: 
    *   Contains predefined functions for common corrective actions (e.g., `restart_supervisor_program_action` which uses `supervisorctl restart`).
    *   Extensible for custom actions.

4.  **`__init__.py`**: 
    *   Exports the main classes and functions for easy import.
    *   Provides `DEFAULT_CHECK_FUNCTIONS` and `DEFAULT_ACTION_FUNCTIONS` dictionaries that map string identifiers (used in configuration) to the actual check/action functions. 

## Flow

1.  **Initialization**: 
    *   A plugin (e.g., `fast_api_plugin`) creates a script (e.g., `health_monitor.py`) that will be managed by `supervisord`.
    *   This script imports `HealthMonitor`, `DEFAULT_CHECK_FUNCTIONS`, and `DEFAULT_ACTION_FUNCTIONS` from the SDK.
    *   It defines a list of `MONITORED_TASKS_CONFIG`. Each entry in this list is a dictionary configuring a specific process to monitor.
    *   It configures root logging for the monitoring script.
    *   It instantiates `HealthMonitor`, passing the task configurations, check functions, action functions, and a base logger name.

2.  **Monitoring Loop (`HealthMonitor.run()`):**
    *   The `HealthMonitor` enters a loop.
    *   For each enabled task in its configuration:
        *   It checks if it's time to perform a health check based on the task's `check_interval_seconds`.
        *   If so, it retrieves the appropriate check function (e.g., `http_get_check`) based on the task's `check_type` and executes it with the task's `check_config`.
        *   **If Healthy**: The consecutive failure count for the task is reset. If the task was previously marked unhealthy, it's marked as recovered.
        *   **If Unhealthy**: The consecutive failure count is incremented.
            *   If the `consecutive_failures` count meets or exceeds the task's `consecutive_failures_threshold` AND the task is not already marked as unhealthy (to prevent repeated actions for an already-failed service):
                *   The task is marked as `is_unhealthy`.
                *   The appropriate action function (e.g., `restart_supervisor_program_action`) is retrieved based on the task's `action_on_failure` and executed with the task's `action_config`.
    *   The loop sleeps for a short interval and repeats.

## How to Use (for a Plugin Developer)

1.  **Ensure SDK is Available**: 
    *   Make sure your plugin's Docker build process copies the `ufm_sdk_tools/src/health_monitoring` directory and the necessary `__init__.py` files (`ufm_sdk_tools/__init__.py`, `ufm_sdk_tools/src/__init__.py`) into the Docker image.
    *   Ensure `/opt/ufm` (or the parent directory where `ufm_sdk_tools` is placed) is in the `PYTHONPATH` within your Docker image.

2.  **Create Plugin's Health Monitor Script**:
    *   In your plugin's scripts directory (e.g., `my_plugin/scripts/health_monitor.py`), create a Python script.

3.  **Implement the Script**:

    ```python
    import logging
    from typing import List, Dict, Any

    # Import from the SDK
    from ufm_sdk_tools.src.health_monitoring import (
        HealthMonitor,
        DEFAULT_CHECK_FUNCTIONS, 
        DEFAULT_ACTION_FUNCTIONS
    )

    # 1. Configure Logging for this script
    LOG_FILE_PATH = "/log/my_plugin_health_monitor.log" # Choose a plugin-specific log path
    logging.basicConfig(
        level=logging.INFO,
        # The TaskLoggerAdapter in the SDK will prepend [TaskName] to the message
        format='%(asctime)s - %(levelname)s - %(name)s - %(message)s', 
        handlers=[
            logging.FileHandler(LOG_FILE_PATH),
            logging.StreamHandler() 
        ]
    )
    BASE_LOGGER_NAME = "MyPluginHealthMonitor" # Plugin-specific base logger name

    # 2. Define Monitored Tasks for your plugin
    MONITORED_TASKS_CONFIG: List[Dict[str, Any]] = [
        {
            "name": "MyProcess1Health",
            "enabled": True,
            "process_name": "my_process_1_supervisor_name",  # Name in supervisord.conf
            "check_interval_seconds": 30,
            "check_type": "http_get",      # Or "command", or a custom key
            "check_config": {              # Config specific to check_type
                "url": "http://localhost:8001/health",
                "timeout": 5,
                "expected_status_code": 200,
            },
            "action_on_failure": "restart_supervisor_program", # Or a custom key
            "action_config": {}, # Config for the action. For restart, supervisor_program_name is auto-filled from process_name if empty
            "consecutive_failures_threshold": 3,
            "recover_after_successes_threshold": 1, # How many successes to be considered recovered (currently 1)
        },
        {
            "name": "MyProcess2CommandCheck",
            "enabled": True,
            "process_name": "my_process_2_supervisor_name",
            "check_interval_seconds": 60,
            "check_type": "command",
            "check_config": {
                "command": ["/usr/local/bin/my_check_script.sh", "--status"],
                "expected_return_code": 0,
            },
            "action_on_failure": "restart_supervisor_program",
            "action_config": {},
            "consecutive_failures_threshold": 2,
        },
        # Add more tasks as needed
    ]

    def main():
        root_logger = logging.getLogger(BASE_LOGGER_NAME)
        root_logger.info(f"Initializing Health Monitor for My Plugin. Logging to: {LOG_FILE_PATH}")

        # 3. Instantiate HealthMonitor
        # You can provide custom check/action functions if needed by creating your own dicts
        # and passing them instead of the defaults.
        monitor = HealthMonitor(
            monitored_tasks=MONITORED_TASKS_CONFIG,
            check_functions=DEFAULT_CHECK_FUNCTIONS, 
            action_functions=DEFAULT_ACTION_FUNCTIONS, 
            base_logger_name=BASE_LOGGER_NAME
        )

        try:
            # 4. Run the monitor
            monitor.run()
        except Exception as e:
            root_logger.critical(f"Health Monitor for My Plugin failed critically: {e}", exc_info=True)

    if __name__ == "__main__":
        main()
    ```

4.  **Configure Supervisord**:
    *   In your plugin's `supervisord.conf`:
        *   Ensure the processes being monitored (e.g., `my_process_1_supervisor_name`) have `autorestart=false` if the health monitor is intended to manage their restarts.
        *   Add a new `[program:health_monitor]` (or a similar unique name) section to run your plugin's `health_monitor.py` script.
        ```ini
        [program:my_plugin_health_monitor] ; Give it a unique name
        command=python3 /scripts/health_monitor.py ; Path to your script within the container
        stdout_logfile=/log/my_plugin_health_monitor_supervisor.log
        redirect_stderr=true
        autostart=true
        autorestart=true ; Important: supervisord should restart the monitor itself if it crashes
        stopsignal=TERM
        ```

5.  **Extend with Custom Checks/Actions (Optional)**:
    *   **Custom Check**: 
        1.  Write a Python function in your plugin's `health_monitor.py` (or a utility module) that takes `config: Dict[str, Any]` and `logger: logging.LoggerAdapter`, and returns `bool`.
        2.  Create a copy of `DEFAULT_CHECK_FUNCTIONS` and add your custom function: 
            `my_check_functions = DEFAULT_CHECK_FUNCTIONS.copy()`
            `my_check_functions["my_custom_check"] = my_custom_check_function`
        3.  Pass `my_check_functions` to the `HealthMonitor` constructor.
        4.  Use `"my_custom_check"` as `check_type` in `MONITORED_TASKS_CONFIG`.
    *   **Custom Action**: Similar process, but for actions and `DEFAULT_ACTION_FUNCTIONS`.

## Logging

*   The plugin-specific health monitor script (e.g., `/log/my_plugin_health_monitor.log`) will contain detailed logs about checks, failures, and actions for each monitored task.
*   The supervisord log for the health monitor script (e.g., `/log/my_plugin_health_monitor_supervisor.log`) will capture stdout/stderr from the monitor script itself, useful for debugging early startup issues or unhandled exceptions within the monitor. 