#!/usr/bin/env python3
"""
Chronos Experimental Data Collection Script
Author: Ojima Abraham
Date: 2025

This script automates the collection of experimental data for the Chronos paper.
It runs the C++ benchmark executable and generates plots from the results.
"""

import subprocess
import time
import json
import statistics
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
import os
import csv

class ChronosExperiment:
    """
    A class to manage and run the Chronos benchmark suite.
    """
    def __init__(self, benchmark_executable_path="./bin/benchmark_chronos"):
        """
        Initializes the experiment class.
        Args:
            benchmark_executable_path (str): The path to the compiled benchmark executable.
        """
        if not os.path.exists(benchmark_executable_path):
            raise FileNotFoundError(
                f"Benchmark executable not found at: {benchmark_executable_path}\n"
                "Please build the project with -DBUILD_BENCHMARKS=ON"
            )
        self.benchmark_executable = benchmark_executable_path
        self.results_dir = "experiment_results"
        os.makedirs(self.results_dir, exist_ok=True)
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    def run_benchmarks(self):
        """
        Runs the C++ benchmark executable and returns the path to the CSV results.
        """
        print("Running C++ benchmark suite...")
        output_file = os.path.join(self.results_dir, f"summary_{self.timestamp}.csv")
        cmd = [self.benchmark_executable, output_file]
        
        try:
            # The C++ program now handles running all benchmarks and saving the CSV.
            # We just need to execute it.
            process = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print(process.stdout)
            if process.stderr:
                print("Benchmark stderr:")
                print(process.stderr)
            print(f"Benchmark data saved to {output_file}")
            return output_file
        except subprocess.CalledProcessError as e:
            print("Error running benchmark executable:")
            print(e.stdout)
            print(e.stderr)
            return None
        except FileNotFoundError:
            print(f"Error: Could not find the benchmark executable at {self.benchmark_executable}")
            return None

    def generate_plots(self, csv_file_path):
        """
        Generates plots for the paper from a CSV results file.
        """
        if not csv_file_path:
            print("CSV file path is invalid. Skipping plot generation.")
            return

        print("Generating plots from benchmark data...")
        
        results = {}
        with open(csv_file_path, 'r') as f:
            reader = csv.reader(f)
            next(reader) # Skip header
            for row in reader:
                test_name, mean, stddev, min_val, max_val, _ = row
                results[test_name] = {
                    'mean': float(mean),
                    'stddev': float(stddev),
                    'min': float(min_val),
                    'max': float(max_val)
                }

        # 1. Partition Overhead Plot
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
        creation_res = results.get('Partition Creation', {})
        release_res = results.get('Partition Release', {})

        ax1.bar(['Mean', 'Min', 'Max'], 
                [creation_res.get('mean', 0), creation_res.get('min', 0), creation_res.get('max', 0)],
                yerr=creation_res.get('stddev', 0), capsize=5, color='skyblue')
        ax1.set_ylabel('Time (ms)')
        ax1.set_title('Partition Creation Overhead')
        ax1.grid(True, linestyle='--', alpha=0.6)

        ax2.bar(['Mean', 'Min', 'Max'], 
                [release_res.get('mean', 0), release_res.get('min', 0), release_res.get('max', 0)],
                yerr=release_res.get('stddev', 0), capsize=5, color='lightcoral')
        ax2.set_ylabel('Time (ms)')
        ax2.set_title('Partition Release Overhead')
        ax2.grid(True, linestyle='--', alpha=0.6)
        
        plt.tight_layout()
        plt.savefig(os.path.join(self.results_dir, f"overhead_{self.timestamp}.png"), dpi=300)
        plt.close()

        # For other plots, we would need more detailed data than the summary CSV.
        # The current C++ benchmark only outputs summary stats.
        # To create scalability plots, the C++ code would need to output per-partition data.
        # I'll add a placeholder message here.
        print("Overhead plot generated.")
        print("Note: Scalability and accuracy plots require more detailed data output from the C++ benchmark.")

    def run_all_experiments(self):
        """
        Run the complete experimental suite.
        """
        print("=== Chronos Experimental Evaluation ===")
        print(f"Timestamp: {self.timestamp}\n")
        
        csv_results_path = self.run_benchmarks()
        if csv_results_path:
            self.generate_plots(csv_results_path)
            
        print("\n=== Experiment Complete ===")

if __name__ == "__main__":
    # The path might need to be adjusted depending on the build system (e.g., on Windows)
    # Or if building in a different directory.
    benchmark_path = os.path.join("build", "bin", "benchmark_chronos")
    if not os.path.exists(benchmark_path):
        print(f"Default benchmark path not found: {benchmark_path}")
        # Fallback for common build configurations
        benchmark_path = os.path.join("build", "benchmark_chronos")
        if not os.path.exists(benchmark_path):
             print("Could not find benchmark executable. Please specify the path.")
             exit(1)

    experiment = ChronosExperiment(benchmark_executable_path=benchmark_path)
    experiment.run_all_experiments()
