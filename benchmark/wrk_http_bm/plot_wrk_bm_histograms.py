"""
Script: plot_wrk_bm_histograms.py

Description:
This script parses `wrk` or `wrk2` output files generated by the 'run_wrk_bm.py' script 
and visualizes key metrics in the form of histograms. The histograms focus on latency percentiles 
and requests per second (RPS) for individual test runs.

Key Features:
- Parses raw `wrk` output files to extract latency percentiles (50%, 75%, 90%, 99%) and RPS.
- Visualizes latency percentiles and RPS metrics as histograms for each test run.
- Saves the output as PNG files for easy analysis and reporting.

Metrics Visualized:
1. Latency Percentiles (50%, 75%, 90%, 99%)
2. Requests Per Second (RPS)

Usage:
1. Run the 'run_wrk_bm.py' script to generate the `wrk` output files.
2. Use this script to parse the output and generate histograms.

Example:
    python plot_wrk_bm_histograms.py -i ./wrk_output.txt -o ./plots
"""

import re
import matplotlib.pyplot as plt
import argparse
import os

def parse_wrk_output(file_path):
    with open(file_path, 'r') as file:
        data = file.read()

    # Regex to extract latency distributions and requests/sec
    regex = r"Running.*?@\s+(.*?)\n.*?(\d+)\sthreads and (\d+)\sconnections.*?Latency Distribution.*?50%\s+([\d\.]+)(ms|s)\s+75%\s+([\d\.]+)(ms|s)\s+90%\s+([\d\.]+)(ms|s)\s+99%\s+([\d\.]+)(ms|s).*?Requests/sec:\s+([\d\.]+)"
    matches = re.findall(regex, data, re.DOTALL)

    results = []
    for match in matches:
        url = match[0]
        threads = int(match[1])
        connections = int(match[2])
        latencies = {
            "50%": float(match[3]) * (1000 if match[4] == "s" else 1),
            "75%": float(match[5]) * (1000 if match[6] == "s" else 1),
            "90%": float(match[7]) * (1000 if match[8] == "s" else 1),
            "99%": float(match[9]) * (1000 if match[10] == "s" else 1),
        }
        rps = float(match[11])  # Requests per second
        results.append({
            "url": url,
            "threads": threads,
            "connections": connections,
            "latencies": latencies,
            "requests_per_sec": rps,
        })

    return results

def plot_histograms(results, output_file, enable_show=False):
    colors = ["blue", "orange", "green", "red", "purple", "brown"]  # Colors for each run

    # Prepare a figure with two subplots
    fig, axes = plt.subplots(2, 1, figsize=(12, 12))

    # Subplot 1: Latency Percentiles
    for idx, result in enumerate(results):
        latencies = result["latencies"]
        percentiles = list(latencies.keys())
        values = list(latencies.values())

        bars = axes[0].bar(
            [f"{percentile} (Run {idx+1})" for percentile in percentiles],
            values,
            color=colors[idx % len(colors)],
            alpha=0.7,
            label=f"{result['url']} (Threads: {result['threads']}, Conns: {result['connections']})",
        )

        # add text labels on the bars
        for bar in bars:
            height = bar.get_height()
            axes[0].text(
                bar.get_x() + bar.get_width() / 2,
                height,
                f'{height:.2f}',
                ha='center',
                va='bottom',
                fontsize=10
            )

    axes[0].set_title("Latency Percentiles Across Runs", fontsize=16)
    axes[0].set_ylabel("Latency (ms)", fontsize=12)
    axes[0].tick_params(axis="x", rotation=45, labelsize=10)
    axes[0].legend(loc="upper left", fontsize=10)
    axes[0].grid(axis="y", linestyle="--", alpha=0.7)

    # Subplot 2: Requests Per Second (RPS)
    for idx, result in enumerate(results):
        rps = result["requests_per_sec"]

        bars = axes[1].bar(
            [f"Run {idx+1}"],
            [rps],
            color=colors[idx % len(colors)],
            alpha=0.7,
            label=f"{result['url']} (Threads: {result['threads']}, Conns: {result['connections']})",
        )

        # Add text labels on the bars
        for bar in bars:
            height = bar.get_height()
            axes[1].text(
                bar.get_x() + bar.get_width() / 2,
                height,
                f'{height:.2f}',
                ha='center',
                va='bottom',
                fontsize=10
            )

    axes[1].set_title("Requests Per Second (RPS) Across Runs", fontsize=16)
    axes[1].set_ylabel("Requests/sec", fontsize=12)
    axes[1].tick_params(axis="x", rotation=45, labelsize=10)
    axes[1].legend(loc="upper left", fontsize=10)
    axes[1].grid(axis="y", linestyle="--", alpha=0.7)

    # Adjust layout and save the figure
    plt.tight_layout()
    plt.savefig(output_file)

    if enable_show:
        plt.show()

    plt.close()
    print(f"Saved histogram to {output_file}")

if __name__ == "__main__":
    # Argument parsing
    parser = argparse.ArgumentParser(description="Generate histograms for wrk output.")
    parser.add_argument("-i", "--input_file", type=str,  help="Path to the wrk output file.")
    parser.add_argument("-o", "--output_folder", type=str, help="Path to the folder to save the plots.")
    args = parser.parse_args()

    # Ensure the output folder exists
    os.makedirs(args.output_folder, exist_ok=True)

    # Parse wrk output and generate plots
    results = parse_wrk_output(args.input_file)

    # Generate output file name based on input file name
    input_file_name = os.path.splitext(os.path.basename(args.input_file))[0]
    output_file_name = input_file_name.replace("_results", "_histogram") + ".png"
    output_file_path = os.path.join(args.output_folder, output_file_name)

    plot_histograms(results, output_file_path)
