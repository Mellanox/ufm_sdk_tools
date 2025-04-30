"""
Script: run_wrk_benchmark.py

Description:
This script wraps the `wrk` and `wrk2` tools, providing a streamlined interface for running 
benchmark tests defined in a YAML configuration file. It automates the execution of benchmarks 
with user-defined test cases and parameters, such as URLs, headers, threads, and connections. 
The script outputs both the raw `wrk`/`wrk2` results and processed CSV files for further analysis.

Key Features:
- Supports both `wrk` (load testing) and `wrk2` (rate-controlled testing).
- Parses a YAML configuration file (`config.yaml`) to define benchmark test cases.
- Outputs `wrk`/`wrk2` raw results and processed CSV files for each test case.
- Simplifies the execution of benchmarks with configurable parameters.

Output Files:
1. **Raw Output**:
   - `wrk`/`wrk2` raw output files for each test case, named `<test_case>_bm_results.txt`.
2. **CSV Files**:
   - Processed results as CSV files, named `<test_case>_bm.csv`.

Usage:
1. Define your benchmark test cases in `config.yaml`.
2. Run this script to execute the benchmarks and generate outputs.

Example:
    python run_wrk_benchmark.py --config_file ./config.yaml --wrk wrk2 --out_path ./results
"""

import os
import subprocess
import re
import yaml
import sys
import ast
import base64
import argparse
# pylint: disable=import-error
import pandas as pd
import matplotlib.pyplot as plt
from urllib.parse import urlparse
from pathlib import Path

def parse_latency(output):
    """
    Parses the output of a latency measurement and extracts the average latency value.
    Args:
        output (str): The output string containing the latency measurement.
    Returns:
        float or None: The average latency value if found, None otherwise.
    """
    # Regex to match Latency Avg value in ms or s
    match = re.search(r"Latency\s+(\d+(?:\.\d+)?)(us|ms|s)", output)
    if match:
        avg_latency = float(match.group(1))
        unit = match.group(2)
        if unit == "s":
            avg_latency *= 1000  # Convert seconds to milliseconds
        return avg_latency
    else:
        print("Latency not found in output")
    return None

def parse_rps(output):
    """
    Parses the output of a benchmark test and extracts the Requests per Second (RPS) value.
    Args:
        output (str): The output of the benchmark test.
    Returns:
        float or None: The RPS value if found in the output, None otherwise.
    """
    match = re.search(r'Requests/sec:\s+([\d\.]+)', output)
    if match:
        rps = float(match.group(1))
        return rps
    return None

def run_wrk(wrk_conf, wrk_v, api, threads, connections, wrk_path):
    """
    Run the wrk benchmark tool with the specified parameters.

    Args:
        wrk_conf the wrk config file
        api: the config api (url, header [optional] etc.)
        threads (int): The number of threads to use for the benchmark.
        connections (int): The number of connections to use for the benchmark.        
        duration (in) duration to run in seconds
        wrk_path (str): The path to the wrk executable.

    Returns:
        str: The output of the wrk benchmark tool.
    """
    url = api["url"]
    headers = []

    duration = wrk_conf.get("duration", "3s")  # Default duration is 3 seconds
    timeout = wrk_conf.get("timeout", "2s")  # Default timeout is 2 seconds

    rate = 0
    if wrk_v == "wrk2":
        rate = int(wrk_conf.get("rate", "10"))  # Default rate 10 (wrk2)

    if "timeout" in api:
        timeout = api["timeout"]

    if "rate" in api:
        rate = api["rate"]

    # Add headers from the configuration
    if "headers" in api:
        for key, value in api["headers"].items():
            # wrk does not handle inline shell commands like $(echo -n 'admin:123456' | base64) such as curl do
            # need to call it explicit e.g. for Authorization: Basic
            if key == "Authorization":
                # Check if it's Basic auth, but only base64-encode the credentials if needed
                if "Basic" not in value and ":" in value:
                    credentials = value.strip()  # user:pass
                    encoded = base64.b64encode(credentials.encode()).decode()
                    headers.extend(['-H', f'"{key}: Basic {encoded}"'])
                else:
                    # If it's token-based, don't encode it
                    headers.extend(['-H', f'"{key}: {value}"'])
            else:
                headers.extend(['-H', f'"{key}: {value}"'])
    cmd = [
        wrk_path,
        f"-t{threads}",
        f"-c{connections}",
        f"-d{duration}",
        "--latency",
        "--timeout",
        f"{timeout}",
        url,        
    ] + headers

    # the -R <rate> option is applicable only for wrk2
    if wrk_v == "wrk2":
        cmd += [f"-R{rate}"]

    # todo : for lua script appenf "-s" lua_script

    command = ' '.join(cmd)
    print("Running command: ", command)

    result = subprocess.run(command, shell=True, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        print("Benchmarking failed. Error message:", result.stderr)
        return None
       
    return result.stdout

def run_benchmark(bm_set, wrk_v, enable_plotting, benchmark_name, threads_connections, out_path, wrk_path="/usr/local/bin/wrk"):
    """
    Run benchmark tests for multiple APIs using wrk.

    Args:
        bm_set the benchmark test params set (dictionary)
        wrk_v wrk version, wrk2 will look for 'rate' attribute in the config
        enable_plotting determines whether generate plotting  
        benchmark_name (str): The name of the benchmark.
        threads_connections:
                list of tuple of threads&connections test case.
                For example [(1, 1), (2, 2), (4, 4), (8, 8), (16, 16)]
        out_path path to store the output files
        wrk_path (str, optional): The path to the wrk executable. Defaults to "/usr/local/bin/wrk".

    Returns:
        None
    """

    apis = bm_set.get("benchmark_apis", [])

    results = pd.DataFrame(columns=["API", "Threads", "Connections", "Latency (ms)", "Requests/sec"])
    outputs = ""

    for api in apis:
        for t, c in threads_connections:
            output = run_wrk(bm_set, wrk_v, api, t, c, wrk_path)

            if output == None:
                print("Failed to run_wrk")
                sys.exit(1)

            outputs += output + "\n"

            try:

                # Parse the output to extract Latency and RPS, then append to the DataFrame
                # This parsing depends on the specific format of wrk's output
                latency = float(parse_latency(output))
                rps = float(parse_rps(output))
            except ValueError:
                # Handle invalid data gracefully, e.g., skip this row
                print(f"Invalid data encountered for API {api['url']} with Threads {t} and Connections {c}")
                continue  # Skip this row

            new_row = pd.DataFrame([{
                "API": api["url"],
                "Threads": t,
                "Connections": c,
                "Latency (ms)": latency,
                "Requests/sec": rps
            }])
            results = pd.concat([results, new_row], ignore_index=True)

    if not os.path.exists(out_path):
        os.makedirs(out_path)

    out_csv = os.path.join(out_path, f"{benchmark_name}_bm.csv")
    out_test_file = os.path.join(out_path, f"{benchmark_name}_bm_results.txt")

    results.to_csv(out_csv, index=False)

    with open(out_test_file, "w") as file:  # 'w' mode overwrites the file if it exists
        file.write(outputs)

    if enable_plotting:
        gen_plotting(benchmark_name, apis, results, out_path)

    print(f"Benchmarking completed. Output files stored to '{out_path}'")

# Plotting
def gen_plotting(benchmark_name, apis, results, out_path):
    for idx, api in enumerate(apis):
        api_results = results[results["API"] == api["url"]]
        plt.figure(figsize=(10, 5))
        plt.subplot(1, 2, 1)
        plt.plot(api_results["Threads"], api_results["Latency (ms)"], label="Latency")
        plt.title("Latency")
        plt.xlabel("Threads")
        plt.ylabel("Latency (ms)")
        plt.legend()

        plt.subplot(1, 2, 2)
        plt.plot(api_results["Threads"], api_results["Requests/sec"], label="Requests/sec")
        plt.title("Requests/sec")
        plt.xlabel("Threads")
        plt.ylabel("Requests/sec")
        plt.legend()

        plt.tight_layout()        
        plt.savefig(f"{out_path}/benchmark_{api['url'].split('/')[-1]}_{benchmark_name}_{idx}.png")
        plt.close()

def parse_arguments():
    """
    Parse command line arguments.

    Returns:
        argparse.Namespace: Parsed command line arguments.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('-t', "--test_name", type=str, help="The name of the benchmark test to run, as specified in the configuration file", required=True)
    parser.add_argument('-w', "--wrk", type=str, help="Specify the tool to use: 'wrk' for load testing or 'wrk2' for controlled rate testing (default: 'wrk')", default="wrk")
    parser.add_argument('-c', "--config_file", type=str, default="./config.yaml", help="Path to the configuration file (default: './config.yaml')", required=True)
    parser.add_argument('-o', "--out_path", type=str, default="/tmp/benchmark/", help="Path to the output folder for saving result files (default: '/tmp/benchmark/')")    
    parser.add_argument('-p', '--plotting', action='store_true', help='Enable plotting')
    ret_args = parser.parse_args()
    return ret_args

def load_config(config_file):
    try:
        with open(config_file, 'r') as file:
            return yaml.safe_load(file)
    except FileNotFoundError:
        print(f"Error: Configuration file '{config_file}' not found.")
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"Error parsing YAML file: {e}")
        sys.exit(1)

def get_wrk_exe(wrk_type):
    # Reading an environment variable
    wrk_path = os.getenv("WRK2_PATH") if wrk_type == "wrk2" else os.getenv("WRK_PATH")
    if not wrk_path:
        print("Error: WRK_PATH|WRK2_PATH is not set.")
        sys.exit(1)

    # Create a Path object for the wrk_path
    wrk_executable = Path(wrk_path)

    # Check if the path exists and is a file
    if not wrk_executable.is_file():
        print(f"Error: The path {wrk_path} does not point to a valid file.")
        sys.exit(1)
    
    # Check if the file is executable
    if not os.access(wrk_executable, os.X_OK):
        print(f"Error: The file {wrk_path} is not executable.")
        sys.exit(1)
            
    return wrk_path

if __name__ == "__main__":

    args = parse_arguments()

    wrk_exe = get_wrk_exe(args.wrk)
    if wrk_exe is None:
        print("Error: WRK_PATH or WRK2_PATH environment variable is not set or invalid. Please export the correct path to the wrk or wrk2 executable.")
        sys.exit(1)

    bm_name = args.test_name
    out_path = args.out_path

    # Load configuration (todo - allow override name in args)
    wrk_config = load_config(args.config_file)
    wrk_bm = wrk_config.get("wrk_bm", {})

    if bm_name not in wrk_bm:
        print(f"Error: Test case '{bm_name}' not found in configuration file.")
        print(f"Available bm : {', '.join(wrk_bm.keys())}")
        sys.exit(1)

    # Extract the threads_connections for the selected test case
    raw_threads_connections = wrk_bm[bm_name].get("threads_connections", [])
    # Convert the raw YAML tuples (stored as strings) back to Python tuples
    threads_connections = [ast.literal_eval(item) for item in raw_threads_connections]

    out_path = wrk_config.get("wrk_output_path", {})
    # append to out path wrk/wrk2
    out_path = os.path.join(out_path, args.wrk)

    run_benchmark(wrk_bm[bm_name], args.wrk, args.plotting, bm_name, threads_connections, out_path=out_path, wrk_path=wrk_exe)
